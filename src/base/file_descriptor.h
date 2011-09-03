#pragma once

namespace cheaproute {

int CheckFdOp(int result, const char* opDescription);
ssize_t CheckFdOp(ssize_t result, const char* opDescription);

class FileDescriptor {
public:
  FileDescriptor()
      : fd_(-1) {
  }
  
  explicit FileDescriptor(int fd)
      : fd_(fd) {
  }
  
  int get() const { return fd_; }
  void set(int value);
  
  ~FileDescriptor();
  
private:
  int fd_;
};

}