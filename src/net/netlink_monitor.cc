

#include "base/common.h"

#include "net/netlink_monitor.h"
#include "base/event_loop.h"
#include "base/file_descriptor.h"
#include "net/netlink_util.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <linux/if_arp.h>

#include <errno.h>
#include <stdio.h>

#include <algorithm>

namespace cheaproute
{


NetlinkMonitor::NetlinkMonitor(EventLoop* loop)
  : loop_(CheckNotNull(loop, "loop")),
    broadcaster_(new Broadcaster<NetlinkListener>()),
    sequence_number_(36),
    last_received_seq_(36) {
}

void NetlinkMonitor::Init() {
  assert(listen_socket_.get() == -1);
  
  listen_socket_.set(CheckFdOp(socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE),
                        "creating netlink socket"));
  
  struct sockaddr_nl addr;
  memset(&addr, 0, sizeof(addr));
  addr.nl_family = AF_NETLINK;
  addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
  CheckFdOp(::bind(listen_socket_.get(), (struct sockaddr*) &addr, sizeof(addr)),
            "binding netlink socket");
  
  ioTask_ = loop_->MonitorFd(listen_socket_.get(), kEvRead, bind(&NetlinkMonitor::HandleRead, this, _1));
  
  BeginLinkQuery();
  BeginAddrQuery(AF_INET);
  BeginAddrQuery(AF_INET6);
}


static Ip6Address GetIp6Address(const ifaddrmsg* ifmsg, const NetlinkAttributeMap& attributes) {
  Ip6Address result;
  NetlinkAttributeMap::const_iterator i;
  if ((i = attributes.find(IFA_ADDRESS)) != attributes.end())
    result = Ip6Address(&i->second[0], i->second.size());
  return result;
}

static Ip4AddressInfo GetIp4AddressInfo(const ifaddrmsg* ifmsg, const NetlinkAttributeMap& attributes) {
  Ip4AddressInfo result;
  NetlinkAttributeMap::const_iterator i;
  if ((i = attributes.find(IFA_ADDRESS)) != attributes.end())
    result.address = Ip4Address(&i->second[0], i->second.size());
  if ((i = attributes.find(IFA_BROADCAST)) != attributes.end())
    result.broadcast = Ip4Address(&i->second[0], i->second.size());
  
  result.prefix_len = ifmsg->ifa_prefixlen;
  return result;
}

NetInterfaceInfo* NetlinkMonitor::GetOrCreateInterface(int index) {
  
  unordered_map<int, shared_ptr<NetInterfaceInfo> >::iterator i = 
    interfaces_by_index_.find(index);
  
  if (i == interfaces_by_index_.end()) {
    i = interfaces_by_index_.insert(make_pair(index, 
                 shared_ptr<NetInterfaceInfo>(new NetInterfaceInfo(index)))).first;
  }

  return i->second.get();
}

namespace {
  
  template<typename T>
  bool SetContains(const set<T>& s, const T& item) {
    typename set<T>::const_iterator i = s.find(item);
    return i == s.end();
  }
}

void NetlinkMonitor::HandleRead(int flags) {
  
  while (true) {
    NetlinkReceiver receiver;
    if (!receiver.ReceiveFromNonBlock(listen_socket_.get()))
      return;
    
    int old_last_received_seq = last_received_seq_;
    
    while (receiver.Next()) {
      const nlmsghdr* nh = receiver.header();
      
      if (nh->nlmsg_seq && (nh->nlmsg_type == NLMSG_DONE || nh->nlmsg_type == NLMSG_ERROR)) {
        last_received_seq_ = nh->nlmsg_seq;
      }
      switch (nh->nlmsg_type)
      {
        case RTM_DELLINK:
        case RTM_NEWLINK: {
          const ifinfomsg* ifmsg = receiver.ifinfomsg();
          
          const NetlinkAttributeMap& attr = receiver.attributes();
          
          NetInterfaceInfo* if_info = GetOrCreateInterface(ifmsg->ifi_index);
          
          NetlinkAttributeMap::const_iterator i;
          if (!if_info->is_public && (i = attr.find(IFLA_IFNAME)) != attr.end()) {
            if_info->name = string(&i->second[0]);
            if_info->is_public = true;
            broadcaster_->Broadcast(
              bind(&NetlinkListener::InterfaceCreated, _1, *if_info));
          }
          
          bool link_active = ifmsg->ifi_flags & IFF_UP;
          if (link_active != if_info->link_active) {
            if_info->link_active = link_active;
            if (if_info->is_public) {
              if (if_info->link_active) {
                broadcaster_->Broadcast(
                  bind(&NetlinkListener::LinkUp, _1, *if_info));
              } else {
                broadcaster_->Broadcast(
                  bind(&NetlinkListener::LinkDown, _1, *if_info));
              }
            }
          }

          break;
        }
        case RTM_NEWADDR:
        case RTM_DELADDR:
        {
          const ifaddrmsg* ifmsg = receiver.ifaddrmsg();
          NetInterfaceInfo* if_info = GetOrCreateInterface(ifmsg->ifa_index);
          const NetlinkAttributeMap& attributes = receiver.attributes();
          
          switch (ifmsg->ifa_family) {
            case AF_INET: {
            
              Ip4AddressInfo address_info = GetIp4AddressInfo(ifmsg, attributes);
              switch (nh->nlmsg_type) {
                
                case RTM_NEWADDR:
                  if (if_info->ip4_addresses.insert(address_info).second &&
                      if_info->is_public) {
                    broadcaster_->Broadcast(
                        bind(&NetlinkListener::Ip4AddressAdded, _1, *if_info, address_info));
                  }
                  break;
                  
                case RTM_DELADDR:
                  if (if_info->ip4_addresses.erase(address_info) && 
                      if_info->is_public) {
                    broadcaster_->Broadcast(
                      bind(&NetlinkListener::Ip4AddressRemoved, _1, *if_info, address_info));
                  }
                  break;
              }
              break;
            }
            case AF_INET6: {
              Ip6Address address = GetIp6Address(ifmsg, attributes);
              switch (nh->nlmsg_type) {
                
                case RTM_NEWADDR:
                  if (if_info->ip6_addresses.insert(address).second && 
                      if_info->is_public) {
                    broadcaster_->Broadcast(
                      bind(&NetlinkListener::Ip6AddressAdded, _1, *if_info, address));
                  }
                  break;
                  
                case RTM_DELADDR:
                  if (if_info->ip6_addresses.erase(address) &&
                      if_info->is_public) {
                    broadcaster_->Broadcast(
                      bind(&NetlinkListener::Ip6AddressRemoved, _1, *if_info, address));
                  }
                  break;
              }
              break;
            } 
          }
        }
          
        break;

        default:
          //printf("Read message %d\n", nh->nlmsg_type);
          break;
      }

    }
    
    while (unsent_messages_.size() > 0 && old_last_received_seq < last_received_seq_) {
      
      SendMessageNow(unsent_messages_.front());
      unsent_messages_.pop_front();
      old_last_received_seq++;
    }
  }
  

}

/*
void Netlink::SetDeviceStatus(int device_index, bool up) {
  NetlinkMessageBuilder nl_builder(RTM_SETLINK, NLM_F_REQUEST);
  nl_builder.header()->nlmsg_seq = ++sequence_number_;
  NetlinkHeader<ifinfomsg> ifheader = nl_builder.CreateHeader<ifinfomsg>();
  ifheader->ifi_family = AF_UNSPEC;
  ifheader->ifi_type = ARPHRD_ETHER;
  ifheader->ifi_index = device_index;
  ifheader->ifi_flags = 
  
}*/


void NetlinkMonitor::BeginAddrQuery(int address_family) {
  NetlinkMessageBuilder nlBuilder(RTM_GETADDR, NLM_F_REQUEST | NLM_F_ROOT);
  nlBuilder.header()->nlmsg_seq = ++sequence_number_;
  NetlinkHeader<ifaddrmsg> ifheader = nlBuilder.CreateHeader<ifaddrmsg>();
  ifheader->ifa_family = static_cast<uint8_t>(address_family);

  SendMessage(nlBuilder.Build());
}

void NetlinkMonitor::BeginLinkQuery() {
  NetlinkMessageBuilder nlBuilder(RTM_GETLINK, NLM_F_REQUEST | NLM_F_ROOT);
  nlBuilder.header()->nlmsg_seq = ++sequence_number_;
  nlBuilder.CreateHeader<ifinfomsg>();

  SendMessage(nlBuilder.Build());
  
}

void NetlinkMonitor::SendMessage(const vector<uint8_t>& data) {
  if (last_received_seq_ + 1 == sequence_number_)
    SendMessageNow(data);
  else {
    unsent_messages_.push_back(data);
  }
}

void NetlinkMonitor::SendMessageNow(const vector<uint8_t>& data)
{
  iovec iov;
  iov.iov_base = const_cast<void*>(static_cast<const void*>(&data[0]));
  iov.iov_len = data.size();
  
  sockaddr_nl sa;
  memset (&sa, 0, sizeof(sa));
  sa.nl_family = AF_NETLINK;
  struct msghdr msg = { (void *)&sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
  ssize_t bytes_sent = sendmsg (listen_socket_.get(), &msg, 0);
  if (bytes_sent != (ssize_t)iov.iov_len) {
    AbortWithPosixError("sendmsg only sent %zu bytes out of %zu", bytes_sent, iov.iov_len);
  }
}

}