
// Copyright 2011 Kor Nielsen
//
// 
#include <stdio.h>
#include <ev.h>
#include <unistd.h>

#include "common.h"

namespace cheaproute
{

void stdin_cb (EV_P_ ev_io *w, int revents)
{
  printf("stdin is ready\n");
}

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


class Invoker
{
public:
  Invoker()
    : loop_(EV_DEFAULT) {}
  
  void Run() {
    ev_run(loop_, 0);
  }
  
  void Schedule(double seconds_from_now, const function<void()>& action)
  {
    ScheduledTask::Start(loop_, seconds_from_now, action);
  }
  
private:
  struct ev_loop* loop_;
};


class Program
{
public:
  Program() {
    some_string_ = "hello";
    invoker_.Schedule(1.0, bind(&Program::DoSomething, this));
  }
  
  void DoSomething() { 
    printf("Something %s\n", some_string_.c_str());
  }
  
  void Run() { invoker_.Run(); }
  
private:
  Program(const Program& other);
  Invoker invoker_;
  string some_string_;
};


int main(int argc, const char* const argv[]) {
  Program program;
  program.Run();
}

}




int main(int argc, const char* const argv[]) {
  return cheaproute::main(argc, argv);
}
