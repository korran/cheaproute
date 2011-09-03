
// Copyright 2011 Kor Nielsen

#include "base/common.h"
#include "base/event_loop.h"

#include <stdio.h>
#include <unistd.h>
#include "net/netlink.h"

namespace cheaproute
{

class InterfaceStatusLogger : NetlinkListener {
public:
  InterfaceStatusLogger(Netlink* netlink) {
    listenerHandle_ = netlink->AddListener(this);
  }
  
  void InterfaceChanged(const NetInterfaceInfo& oldInfo, const NetInterfaceInfo& newInfo) {
    printf("Interface changed: %s\n", oldInfo.name.c_str());
  }
  
  void Ip4AddressAdded(const NetInterfaceInfo& info, const Ip4AddressInfo& address) {
    printf("[%d] added IP address %s\n", info.index, address.ToString().c_str());
  }
  void Ip4AddressRemoved(const NetInterfaceInfo& info, const Ip4AddressInfo& address) {
    printf("[%d] removed IP address %s\n", info.index, address.ToString().c_str());
  }
  void Ip6AddressAdded(const NetInterfaceInfo& info, const Ip6Address& address) {
    printf("[%d] added IP address %s\n", info.index, address.ToString().c_str());
  }
  void Ip6AddressRemoved(const NetInterfaceInfo& info, const Ip6Address& address) {
    printf("[%d] removed IP address %s\n", info.index,  address.ToString().c_str());
  }
  void LinkUp(const NetInterfaceInfo& info) {
    printf("[%d] %s link up\n", info.index, info.name.c_str());
  }
  void LinkDown(const NetInterfaceInfo& info) {
    printf("[%d] %s link down\n", info.index, info.name.c_str());
  }
  
private:
  shared_ptr<ListenerHandle> listenerHandle_;
};
  
class Program
{
public:
  Program() {
    loop_.reset(new EventLoop());
    netlink_.reset(new Netlink(loop_.get()));
    interface_status_logger_.reset(new InterfaceStatusLogger(netlink_.get()));
  }
  
  void Init() {
    netlink_->Init();
  }
  
  void AddInternalInterface(const string& ifname) {
  }
  
  void Run() { 
    loop_->Run(); 
  }
  
private:
  Program(const Program& other);
  scoped_ptr<EventLoop> loop_;
  scoped_ptr<Netlink> netlink_;
  scoped_ptr<InterfaceStatusLogger> interface_status_logger_;
};

}

int main(int argc, const char *const argv[]) {
  cheaproute::Program program;
  program.Init();
  program.Run();
}
