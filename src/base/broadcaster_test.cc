#include "base/common.h"

#include "gtest/gtest.h"
#include "test_util/stream.h"

#include "base/broadcaster.h"

#include <sstream>

namespace cheaproute {

class MyListener {
public:
  virtual void HandleClickEvent(int x, int y) = 0;
};

class FakeListener : public MyListener {
public:
  FakeListener(Broadcaster<MyListener>* broadcaster) {
    assert(broadcaster);
    listener_handle_ = broadcaster->AddListener(this);
  }
  
  virtual void HandleClickEvent(int x, int y) {
    ss_ << "Click(" << x << ", " << y << ")\n";
  }
    
  string ToString() const {
    return ss_.str();
  }
  
private:
  std::stringstream ss_;
  shared_ptr<ListenerHandle> listener_handle_;
};
  
TEST(Broadcaster, ListeneredDestroyedBeforeBroadcaster) {
  shared_ptr<Broadcaster<MyListener> > broadcaster(new Broadcaster<MyListener>());
  
  {
    scoped_ptr<FakeListener> listener1(new FakeListener(broadcaster.get()));
    broadcaster->Broadcast(bind(
      &MyListener::HandleClickEvent, _1, 1, 2));
    ASSERT_EQ("Click(1, 2)\n", listener1->ToString());
  }
  broadcaster->Broadcast(bind(
      &MyListener::HandleClickEvent, _1, 3, 4));
}

  
TEST(Broadcaster, ListeneredDestroyedBeforeBroadcaster_MultipleListeners) {
  shared_ptr<Broadcaster<MyListener> > broadcaster(new Broadcaster<MyListener>());
  
  {
    scoped_ptr<FakeListener> listener1(new FakeListener(broadcaster.get()));
    scoped_ptr<FakeListener> listener2(new FakeListener(broadcaster.get()));
    broadcaster->Broadcast(bind(
      &MyListener::HandleClickEvent, _1, 1, 2));
    ASSERT_EQ("Click(1, 2)\n", listener1->ToString());
    ASSERT_EQ("Click(1, 2)\n", listener2->ToString());
    listener1.reset();
    broadcaster->Broadcast(bind(
      &MyListener::HandleClickEvent, _1, 3, 4));
    ASSERT_EQ("Click(1, 2)\nClick(3, 4)\n", listener2->ToString());
  }
  broadcaster->Broadcast(bind(
      &MyListener::HandleClickEvent, _1, 5, 6));
}

TEST(Broadcaster, BroadcasterDestroyedBeforeSingleListener) {
  shared_ptr<Broadcaster<MyListener> > broadcaster(new Broadcaster<MyListener>());
  
  scoped_ptr<FakeListener> listener1(new FakeListener(broadcaster.get()));
  broadcaster->Broadcast(bind(
    &MyListener::HandleClickEvent, _1, 1, 2));
  broadcaster.reset();
  ASSERT_EQ("Click(1, 2)\n", listener1->ToString());
}

TEST(Broadcaster, BroadcasterDestroyedBeforeMultipleListeners) {
  shared_ptr<Broadcaster<MyListener> > broadcaster(new Broadcaster<MyListener>());
  
  scoped_ptr<FakeListener> listener1(new FakeListener(broadcaster.get()));
  scoped_ptr<FakeListener> listener2(new FakeListener(broadcaster.get()));
  broadcaster->Broadcast(bind(
    &MyListener::HandleClickEvent, _1, 1, 2));
  broadcaster.reset();
  ASSERT_EQ("Click(1, 2)\n", listener1->ToString());
  ASSERT_EQ("Click(1, 2)\n", listener2->ToString());
}
}