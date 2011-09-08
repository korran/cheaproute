
#include "net/netlink.h"
#include "net/netlink_util.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <linux/if_arp.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "ip_address.h"

namespace cheaproute {

Netlink::Netlink()
  : sequence_number_(0) {
}

static void AbortIfNextMessageIsNetlinkError(NetlinkReceiver* receiver, 
                                             const char* message) {
  if (receiver->Next()) {
    if (receiver->header()->nlmsg_type == NLMSG_ERROR &&
        receiver->nlmsgerr()->error != 0) {
      AbortWithPosixError(-receiver->nlmsgerr()->error, "%s", message);
    }
  }
}
  
void Netlink::Init() {
  socket_.set(CheckFdOp(socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE),
                        "creating netlink socket"));
  
  struct sockaddr_nl addr;
  memset(&addr, 0, sizeof(addr));
  addr.nl_family = AF_NETLINK;
  CheckFdOp(::bind(socket_.get(), (struct sockaddr*) &addr, sizeof(addr)),
            "binding netlink socket");
}

void Netlink::SetDeviceStatus(int device_index, bool up) {
  NetlinkMessageBuilder nl_builder(RTM_SETLINK, NLM_F_REQUEST | NLM_F_ACK);
  nl_builder.header()->nlmsg_seq = ++sequence_number_;
  
  NetlinkHeader<ifinfomsg> ifheader = nl_builder.CreateHeader<ifinfomsg>();
  ifheader->ifi_family = AF_UNSPEC;
  ifheader->ifi_type = ARPHRD_ETHER;
  ifheader->ifi_index = device_index;
  ifheader->ifi_flags |= up ? IFF_UP : 0;
  ifheader->ifi_change = IFF_UP;
  
  SendNetlinkMessage(socket_.get(), nl_builder.Build());
  
  NetlinkReceiver receiver;
  receiver.ReceiveFrom(socket_.get());
  
  AbortIfNextMessageIsNetlinkError(&receiver, "Unable to set 'up' status for device");
}

void Netlink::SetDeviceIp4AddressInfo(int device_index, const cheaproute::Ip4AddressInfo& address_info) {

  NetlinkMessageBuilder nl_builder(RTM_NEWADDR, NLM_F_REQUEST | NLM_F_ACK);
  nl_builder.header()->nlmsg_seq = ++sequence_number_;
  
  NetlinkHeader<ifaddrmsg> ifheader = nl_builder.CreateHeader<ifaddrmsg>();
  ifheader->ifa_family = AF_INET;
  ifheader->ifa_prefixlen = address_info.prefix_len;
  ifheader->ifa_index = device_index;
  
  const Ip4Address addr = address_info.address;
  const Ip4Address broadcast = address_info.broadcast;
  
  nl_builder.AddAttribute(IFA_LOCAL, &addr.addr, sizeof(addr.addr));
  nl_builder.AddAttribute(IFA_ADDRESS, &addr.addr, sizeof(addr.addr));
  nl_builder.AddAttribute(IFA_BROADCAST, &broadcast.addr, sizeof(broadcast.addr));
  SendNetlinkMessage(socket_.get(), nl_builder.Build());
  
  NetlinkReceiver receiver;
  receiver.ReceiveFrom(socket_.get());
  
  AbortIfNextMessageIsNetlinkError(&receiver, "Unable to set 'up' status for device");
}


}