
#include "base/event_loop.h"

#include <ev.h>

namespace cheaproute
{

class IoTask {
public:
  static shared_ptr<IoTask> Create(struct ev_loop* loop, 
                                   int fd,
                                   int flags, 
                                   const function<void(int)>& func) {
    shared_ptr<IoTask> result(new IoTask(loop, fd, flags, func));
    return result;
  }
  
  ~IoTask() {
    ev_io_stop(loop_, &io_);
  }
  
private:
  IoTask(struct ev_loop* loop, int fd, int flags, const function<void(int)>& func) 
      : loop_(CheckNotNull(loop, "loop")),
        func_(func) {
    int evFlags = 0;
    if (flags & kEvRead)
      evFlags |= EV_READ;
    if (flags & kEvWrite)
      evFlags |= EV_WRITE;
    
    ev_io_init (&io_, &IoTask::HandleFunctionRead, fd, evFlags);
    io_.data = this;
    ev_io_start(loop, &io_);
  }
  

  
  static void HandleFunctionRead(struct ev_loop* loop, ev_io *w, int revents) {
    IoTask* thiss = static_cast<IoTask*>(w->data);
    int flags = 0;
    if (revents & EV_READ)
      flags |= kEvRead;
    else if (revents & EV_WRITE) 
      flags |= kEvWrite;
    thiss->func_(revents);
  }
  
  struct ev_loop* loop_;
  function<void(int)> func_;
  struct ev_io io_;
};

  
class ScheduledTask {
public:
  static void Start(struct ev_loop* loop, double seconds_from_now, const function<void()>& func) {
    ScheduledTask* task = new ScheduledTask(seconds_from_now, func);
    ev_timer_start(loop, &task->timer_);
  }
  
private:
  ScheduledTask(double seconds_from_now, const function<void()>& func)
    : func_(func)
  {
    ev_timer_init(&timer_, &ScheduledTask::HandleFunctionTimeout, seconds_from_now, 0.0);
    timer_.data = this;
  }
  
  static void HandleFunctionTimeout(struct ev_loop* loop, ev_timer *w, int revents)
  {
    ScheduledTask* task = static_cast<ScheduledTask*>(w->data);
    task->func_();
    delete task;
  }
  
  ev_timer timer_;
  function<void()> func_;
};



EventLoop::EventLoop()
  : loop_(EV_DEFAULT) {}

void EventLoop::Run() {
  ev_loop(loop_, 0);
}

void EventLoop::Schedule(double seconds_from_now, const function<void()>& func) {
  ScheduledTask::Start(loop_, seconds_from_now, func);
}

shared_ptr<IoTask> EventLoop::MonitorFd(int fd, int flags, const function<void(int)>& action) {
  return IoTask::Create(loop_, fd, flags, action);
}
  

}