#pragma once

#include "base/common.h"

namespace cheaproute {

class ListenerHandle {
public:
  virtual ~ListenerHandle() {};
};



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
  vector<TListener*> listeners_;
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
  listeners_.push_back(listener);
  return shared_ptr<ListenerHandle>(
    new MyListenerHandle<TListener>(weak_ptr<Broadcaster<TListener> >(this->shared_from_this()), listener));
}

}