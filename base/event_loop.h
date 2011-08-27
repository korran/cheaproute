#pragma once

#include "common.h"

struct ev_loop;

namespace cheaproute
{
  
class EventLoop
{
public:
  EventLoop();
  
  void Run();
  void Schedule(double seconds_from_now, const function<void()>& action);
  
private:
  struct ev_loop* loop_;
};

}