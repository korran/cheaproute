
#include "base/common.h"
#include "base/file_descriptor.h"

#include <unistd.h>
#include <stdio.h>

namespace cheaproute {
  
int CheckFdOp(int result, const char* opDescription) {
  if (result == -1) {
    AbortWithPosixError("Fatal error while %s", opDescription);
  }
  return result;
}

void FileDescriptor::set(int value) {
  if (fd_ != -1) {
    close(fd_);
  }
  fd_ = value;
}

FileDescriptor::~FileDescriptor()
{

}

}