#pragma once

#include "base/common.h"
#include <algorithm>

namespace cheaproute {

class ListenerHandle {
public:
  virtual ~ListenerHandle() {};
};

namespace {
  template<typename TListener>
  class MyListenerHandle ;
}

template<typename TListener>
class Broadcaster : public enable_shared_from_this<Broadcaster<TListener> > {
public:
  template<typename TAction>
  void Broadcast(TAction action) {
    for (typename vector<TListener*>::iterator i = listeners_.begin(); i != listeners_.end(); i++) {
      action(*i);
    }
  }
  
  shared_ptr<ListenerHandle> AddListener(TListener* listener);
  
private:
  void RemoveListener(TListener* listener) {
    listeners_.erase(
        std::remove(listeners_.begin(), listeners_.end(), listener));
  }
  vector<TListener*> listeners_;
  
  friend class MyListenerHandle<TListener>;
};

namespace {
  template<typename TListener>
  class MyListenerHandle : public ListenerHandle{
  public:
    MyListenerHandle(weak_ptr<Broadcaster<TListener> > broadcaster, TListener* listener)
        : broadcaster_(broadcaster),
          listener_(listener) {
    }
      
    virtual ~MyListenerHandle() {
      shared_ptr<Broadcaster<TListener> > sp_broadcaster = broadcaster_.lock();
      if (sp_broadcaster.get())
        sp_broadcaster->RemoveListener(listener_);
    }
    
  private:
    weak_ptr<Broadcaster<TListener> > broadcaster_;
    TListener* listener_;
  };
}

template<typename TListener>
shared_ptr<ListenerHandle> Broadcaster<TListener>::AddListener(TListener* listener)
{
  CheckNotNull(listener, "listener");
  assert(std::find(listeners_.begin(), listeners_.end(), listener) == 
        listeners_.end());
  listeners_.push_back(listener);
  return shared_ptr<ListenerHandle>(
    new MyListenerHandle<TListener>(weak_ptr<Broadcaster<TListener> >(this->shared_from_this()), listener));
}

}