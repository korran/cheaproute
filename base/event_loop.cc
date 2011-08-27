
#include "base/event_loop.h"

#include <ev.h>

namespace cheaproute
{

class ScheduledTask
{
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
  ev_run(loop_, 0);
}

void EventLoop::Schedule(double seconds_from_now, const function<void()>& func) {
  ScheduledTask::Start(loop_, seconds_from_now, func);
}

}