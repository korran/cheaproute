#pragma once

#include "common.h"

struct ev_loop;

namespace cheaproute
{

const int kEvRead = 0x01;
const int kEvWrite = 0x02;

class IoTask;

class EventLoop
{
public:
  EventLoop();
  
  void Run();
  void Schedule(double seconds_from_now, const function<void()>& action);
  
  // Note: The monitor will only work while the returned IoTask is not destroyed
  shared_ptr<IoTask> MonitorFd(int fd, int flags, const function<void(int)>& action);
  
private:
  struct ev_loop* loop_;
};

}