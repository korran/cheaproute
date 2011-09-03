
#include "base/common.h"

#include "net/netlink.h"
#include "base/event_loop.h"
#include "base/file_descriptor.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <errno.h>

#include <algorithm>

#include <stdio.h>

namespace cheaproute
{
typedef unordered_map<int, vector<char> > NetlinkAttributeMap;

namespace {
  template<typename T>
  class NetlinkHeader {
  public:
    NetlinkHeader(vector<char>* buffer, size_t offset) 
      : buffer_(buffer),
        offset_(offset) {
    }
    
    T* get() { return reinterpret_cast<T*>(&(*buffer_)[offset_]); }
    T* operator->() { return get(); }
    
  private:
    vector<char>* buffer_;
    size_t offset_;
  };

  class NetlinkMessageBuilder {
  public:
    NetlinkMessageBuilder(uint16_t type, uint16_t flags) 
        : mainHeader_(&message_, 0) {
      message_.resize(sizeof(nlmsghdr));
      header()->nlmsg_type = type;
      header()->nlmsg_flags = flags;
    }
    nlmsghdr* header() { return mainHeader_.get(); }
    
    template<typename T>
    NetlinkHeader<T> CreateHeader() {
      size_t offset = message_.size();
      message_.resize(message_.size() + sizeof(T));
      return NetlinkHeader<T>(&message_, offset);
    }
    
    const vector<char>& Build() {
      header()->nlmsg_len = static_cast<uint32_t>(message_.size());
      return message_;
    }
    
  private:
    vector<char> message_;
    NetlinkHeader<nlmsghdr> mainHeader_;
  };
}

Netlink::Netlink(EventLoop* loop)
  : loop_(CheckNotNull(loop, "loop")),
    broadcaster_(new Broadcaster<NetlinkListener>()),
    sequence_number_(0),
    last_received_seq_(0) {
}

void Netlink::Init() {
  assert(socket_.get() == -1);
  
  socket_.set(CheckFdOp(socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE),
                        "creating netlink socket"));
  
  struct sockaddr_nl addr;
  memset(&addr, 0, sizeof(addr));
  addr.nl_family = AF_NETLINK;
  addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
  CheckFdOp(::bind(socket_.get(), (struct sockaddr*) &addr, sizeof(addr)),
            "binding netlink socket");
  
  ioTask_ = loop_->MonitorFd(socket_.get(), kEvRead, bind(&Netlink::HandleRead, this, _1));
  
  BeginLinkQuery();
  BeginAddrQuery(AF_INET);
  BeginAddrQuery(AF_INET6);
}

static NetlinkAttributeMap RetrieveNetlinkAttributes(size_t rtasize, struct rtattr* rta)
{
  NetlinkAttributeMap result;
  
  for (; RTA_OK(rta, rtasize); 
       rta = RTA_NEXT(rta, rtasize)) {
    vector<char>& data = result[rta->rta_type];
    data.resize(RTA_PAYLOAD(rta));
    memcpy(&data[0], RTA_DATA(rta), data.size());
  }
  
  return result;
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
  
  result.netmask = Ip4Address(0xffffffff << (32 - ifmsg->ifa_prefixlen));
  return result;
}

void Netlink::HandleRead(int flags) {
  
  char buf[4096];
  while (true) {
    struct msghdr msg;
    struct sockaddr_nl addr;
    struct iovec iov;
    
    iov.iov_base = &buf[0];
    iov.iov_len = sizeof(buf);

    msg.msg_name = &addr;
    msg.msg_namelen = sizeof(addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    ssize_t len = recvmsg(socket_.get(), &msg, MSG_DONTWAIT);
    if (len == -1) {
      if (errno == EAGAIN) {
        return;
      }
      CheckFdOp(len, "receiving message from netlink socket");
    }
    int old_last_received_seq = last_received_seq_;
    
    for (struct nlmsghdr* nh = (struct nlmsghdr*) buf; 
         NLMSG_OK(nh, len); 
         nh = NLMSG_NEXT(nh, len)) {
      last_received_seq_ = nh->nlmsg_seq;
      switch (nh->nlmsg_type)
      {
        case RTM_DELLINK:
        case RTM_NEWLINK: {
          ifinfomsg* ifmsg = (ifinfomsg*) NLMSG_DATA(nh);
          NetInterfaceInfo if_info;
          if_info.index = ifmsg->ifi_index;
          NetlinkAttributeMap attributes = RetrieveNetlinkAttributes(IFLA_PAYLOAD(nh), IFLA_RTA(ifmsg));
          NetlinkAttributeMap::const_iterator i;
          if ((i = attributes.find(IFLA_IFNAME)) != attributes.end()) {
            if_info.name = string(&i->second[0], i->second.size());
          }
          if_info.link_active = ifmsg->ifi_flags & IFF_UP;
          if (if_info.link_active) {
            broadcaster_->Broadcast(
              bind(&NetlinkListener::LinkUp, _1, if_info));
          } else {
            broadcaster_->Broadcast(
              bind(&NetlinkListener::LinkDown, _1, if_info));
          }
          break;
        }
        case RTM_NEWADDR:
        case RTM_DELADDR:
        {
          struct ifaddrmsg* ifmsg = (struct ifaddrmsg*) NLMSG_DATA(nh);
          NetInterfaceInfo if_info;
          if_info.index = ifmsg->ifa_index  ;
          NetlinkAttributeMap attributes = RetrieveNetlinkAttributes(IFA_PAYLOAD(nh), IFA_RTA(ifmsg));
          switch (ifmsg->ifa_family) {
            case AF_INET: {
              Ip4AddressInfo address_info = GetIp4AddressInfo(ifmsg, attributes);
              switch (nh->nlmsg_type) {
                case RTM_NEWADDR:
                  broadcaster_->Broadcast(
                    bind(&NetlinkListener::Ip4AddressAdded, _1, if_info, address_info));
                  break;
                case RTM_DELADDR:
                  broadcaster_->Broadcast(
                    bind(&NetlinkListener::Ip4AddressRemoved, _1, if_info, address_info));
                  break;
              }
              break;
            }
            case AF_INET6: {
              Ip6Address address = GetIp6Address(ifmsg, attributes);
              switch (nh->nlmsg_type) {
                case RTM_NEWADDR:
                  broadcaster_->Broadcast(
                    bind(&NetlinkListener::Ip6AddressAdded, _1, if_info, address));
                  break;
                case RTM_DELADDR:
                  broadcaster_->Broadcast(
                    bind(&NetlinkListener::Ip6AddressRemoved, _1, if_info, address));
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


void Netlink::BeginAddrQuery(int address_family) {
  NetlinkMessageBuilder nlBuilder(RTM_GETADDR, NLM_F_REQUEST | NLM_F_ROOT);
  nlBuilder.header()->nlmsg_seq = ++sequence_number_;
  NetlinkHeader<ifaddrmsg> ifheader = nlBuilder.CreateHeader<ifaddrmsg>();
  ifheader->ifa_family = static_cast<uint8_t>(address_family);

  SendMessage(nlBuilder.Build());
}

void Netlink::BeginLinkQuery() {
  NetlinkMessageBuilder nlBuilder(RTM_GETLINK, NLM_F_REQUEST | NLM_F_ROOT);
  nlBuilder.header()->nlmsg_seq = ++sequence_number_;
  nlBuilder.CreateHeader<ifinfomsg>();

  SendMessage(nlBuilder.Build());
  
}

void Netlink::SendMessage(const vector<char>& data) {
  if (last_received_seq_ + 1 == sequence_number_)
    SendMessageNow(data);
  else 
    unsent_messages_.push_back(data);
}

void Netlink::SendMessageNow(const vector<char>& data)
{
  iovec iov;
  iov.iov_base = const_cast<void*>(static_cast<const void*>(&data[0]));
  iov.iov_len = data.size();
  
  sockaddr_nl sa;
  memset (&sa, 0, sizeof(sa));
  sa.nl_family = AF_NETLINK;
  struct msghdr msg = { (void *)&sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
  ssize_t bytes_sent = sendmsg (socket_.get(), &msg, 0);
  if (bytes_sent != (ssize_t)iov.iov_len) {
    AbortWithPosixError("sendmsg only sent %zu bytes out of %zu", bytes_sent, iov.iov_len);
  }
}

}