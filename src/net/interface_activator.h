#pragma once

#include "base/common.h"
#include "base/broadcaster.h"
#include "net/netlink_monitor.h"

namespace cheaproute {
  
class InterfaceActivator : NetlinkListener {
public:
  InterfaceActivator(Netlink* netlink, NetlinkMonitor* netlink_monitor) {
    netlink_ = CheckNotNull(netlink, "netlink");
    CheckNotNull(netlink_monitor, "netlink_monitor");
    listenerHandle_ = netlink_monitor->AddListener(this);
  }
  void ConfigureInterface(const string& ifname, Ip4AddressInfo address_info) {
    interface_ip4_info_[ifname] = address_info;
  }
  
private:
  void InterfaceCreated(const NetInterfaceInfo& info) {
    unordered_map<string, Ip4AddressInfo>::const_iterator i = 
        interface_ip4_info_.find(info.name);
        
    if (i != interface_ip4_info_.end()) {
      netlink_->SetDeviceStatus(info.index, true);
      netlink_->SetDeviceIp4AddressInfo(info.index, i->second);
    }
  }
  
  Netlink* netlink_;
  unordered_map<string, Ip4AddressInfo> interface_ip4_info_;
  shared_ptr<ListenerHandle> listenerHandle_;
};
  
}