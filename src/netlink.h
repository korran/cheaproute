
#include "base/common.h"
#include "base/file_descriptor.h"
#include "base/ip_address.h"
#include "base/broadcaster.h"

struct iovec;
namespace cheaproute
{

class EventLoop;
class IoTask;

struct NetInterfaceInfo {
  string name;
  int index;
  bool link_active;
  vector<Ip4Address> ip4_addresses;
  vector<Ip6Address> ip6_addresses;
};

class NetlinkListener {
public:
  virtual void InterfaceChanged(const NetInterfaceInfo& oldInfo, const NetInterfaceInfo& newInfo) = 0;
  virtual void Ip4AddressAdded(const NetInterfaceInfo& info, const Ip4AddressInfo& address) = 0;
  virtual void Ip4AddressRemoved(const NetInterfaceInfo& info, const Ip4AddressInfo& address) = 0;
  virtual void Ip6AddressAdded(const NetInterfaceInfo& info, const Ip6Address& address) = 0;
  virtual void Ip6AddressRemoved(const NetInterfaceInfo& info, const Ip6Address& address) = 0;
  virtual void LinkUp(const NetInterfaceInfo& info) = 0;
  virtual void LinkDown(const NetInterfaceInfo& info) = 0;
};


class Netlink
{
public:
  explicit Netlink(EventLoop* loop);
  
  void Init();
  
  shared_ptr<ListenerHandle> AddListener(NetlinkListener* listener) {
    return broadcaster_->AddListener(listener);
  }
  
private:
  void HandleRead(int flags);
  void BeginLinkQuery();
  void BeginAddrQuery(int address_family);
  void SendMessage(const vector<char>& msg);
  void SendMessageNow(const vector<char>& msg);
  
  FileDescriptor socket_;
  EventLoop* loop_;
  
  shared_ptr<IoTask> ioTask_;
  shared_ptr<Broadcaster<NetlinkListener> > broadcaster_;
  int sequence_number_;
  int last_received_seq_;
  deque<vector<char> > unsent_messages_;
};
}