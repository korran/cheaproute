#pragma once

#include "base/common.h"
#include "base/file_descriptor.h"
#include "net/ip_address.h"
#include "base/broadcaster.h"

struct iovec;
struct ifinfomsg;
struct ifaddrmsg;
namespace cheaproute
{

class EventLoop;
class IoTask;

struct NetInterfaceInfo {
  explicit NetInterfaceInfo(int index)
    : index(index),
      link_active(false),
      is_public(false) {
  }
    
  const int index;
  string name;
  bool link_active;
  bool is_public;
  set<Ip4AddressInfo> ip4_addresses;
  set<Ip6Address> ip6_addresses;
};

class NetlinkListener {
public:
  virtual void InterfaceChanged(const NetInterfaceInfo& oldInfo, const NetInterfaceInfo& newInfo) {};
  virtual void Ip4AddressAdded(const NetInterfaceInfo& info, const Ip4AddressInfo& address) {};
  virtual void Ip4AddressRemoved(const NetInterfaceInfo& info, const Ip4AddressInfo& address) {};
  virtual void Ip6AddressAdded(const NetInterfaceInfo& info, const Ip6Address& address) {};
  virtual void Ip6AddressRemoved(const NetInterfaceInfo& info, const Ip6Address& address) {};
  virtual void LinkUp(const NetInterfaceInfo& info) {};
  virtual void LinkDown(const NetInterfaceInfo& info) {};
  virtual void InterfaceCreated(const NetInterfaceInfo& info) {};
};


class NetlinkMonitor
{
public:
  explicit NetlinkMonitor(EventLoop* loop);
  
  void Init();
  
  shared_ptr<ListenerHandle> AddListener(NetlinkListener* listener) {
    return broadcaster_->AddListener(listener);
  }
  
private:
  NetInterfaceInfo* GetOrCreateInterface(int index);
  void HandleRead(int flags);
  void BeginLinkQuery();
  void BeginAddrQuery(int address_family);
  void SendMessage(const vector<uint8_t>& msg);
  void SendMessageNow(const vector<uint8_t>& msg);
  
  FileDescriptor listen_socket_;
  
  EventLoop* loop_;
  
  shared_ptr<IoTask> ioTask_;
  shared_ptr<Broadcaster<NetlinkListener> > broadcaster_;
  int sequence_number_;
  int last_received_seq_;
  deque<vector<uint8_t> > unsent_messages_;
  unordered_map<string, shared_ptr<NetInterfaceInfo> > interfaces_by_name_;
  unordered_map<int, shared_ptr<NetInterfaceInfo> > interfaces_by_index_;
};
}