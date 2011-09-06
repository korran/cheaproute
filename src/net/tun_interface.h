
#include "base/common.h"
#include "base/file_descriptor.h"
#include "base/broadcaster.h"

namespace cheaproute {
  class EventLoop;
  class IoTask;
  
  class TunListener {
  public:
    virtual void PacketReceived(const void* data, size_t size) = 0;
  };
  
  class TunInterface {
  public:
    TunInterface(EventLoop* loop, const string& name);
    
    shared_ptr<ListenerHandle> AddListener(TunListener* listener) {
      return broadcaster_->AddListener(listener);
    }
    void SendPacket(const void* data, size_t size);
    
  private:
    void HandleRead(int flags);
    
    EventLoop* loop_;
    FileDescriptor fd_;
    shared_ptr<Broadcaster<TunListener> > broadcaster_;
    shared_ptr<IoTask> ioTask_;
  };
}