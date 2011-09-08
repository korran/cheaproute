#pragma once

#include "base/common.h"

namespace cheaproute {
  
class InputStream {
public:
  virtual ~InputStream() {}
  virtual ssize_t Read(void* buf, size_t count) = 0;
};

class OutputStream {
public:
  virtual ~OutputStream() {}
  virtual void Write(const void* buf, size_t count) = 0;
  virtual void Flush() = 0;
};

class BufferedInputStream : public InputStream {
public:
  BufferedInputStream(TransferredOwnershipPtr<InputStream> delegatee, 
                      size_t buffer_size);
  
  ssize_t Read(void* buf, size_t count);
  
  // returns -1 if we hit the end of the stream
  int Read() {
    if (pos_ != end_) {
      return *(pos_++);
    } else {
      return ReadSlowPath();
    }
  }
  
  int Peek() {
    if (pos_ != end_) {
      return *pos_;
    } else {
      return PeekSlowPath();
    }
  }
  
private:
  int ReadSlowPath();
  int PeekSlowPath();
  ssize_t FillBuffer();
  
  scoped_ptr<InputStream> delegatee_;
  vector<uint8_t> buffer_;
  vector<uint8_t>::iterator pos_;
  vector<uint8_t>::iterator end_;
};

class BufferedOutputStream : public OutputStream {
public:
  BufferedOutputStream(TransferredOwnershipPtr<OutputStream> delegatee, size_t buffer_size);
  
  void Write(char ch) {
    *pos_++ = ch;
    if (pos_ == end_) {
      Flush();
    }
  }
  void Write(const void* buf, size_t size);
  void Flush();
  
private:
  scoped_ptr<OutputStream> delegatee_;
  vector<uint8_t> buffer_;
  vector<uint8_t>::iterator pos_;
  vector<uint8_t>::iterator end_;
};

class MemoryInputStream : public InputStream {
public:
  MemoryInputStream(const void* data, size_t size);
  virtual ssize_t Read(void* buf, size_t count);
  
private:
  vector<uint8_t> data_;
  size_t pos_;
};

class MemoryOutputStream : public OutputStream {
public:
  MemoryOutputStream();
  
  virtual void Write(const void* buf, size_t count);
  virtual void Flush() {}
  
  const void* ptr() const { return buffer_.empty() ? NULL : &buffer_[0]; }
  const size_t size() const { return pos_; }
  
private:
  vector<uint8_t> buffer_;
  size_t pos_;
};

class FileInputStream : public InputStream {
public:
  FileInputStream(int fd, bool take_fd_ownership)
    : fd_(fd), take_fd_ownership_(take_fd_ownership) {
  }
  FileInputStream(const char* file_path);
  virtual ~FileInputStream();
    
  ssize_t Read(void* buf, size_t count);
  
private:
  int fd_;
  bool take_fd_ownership_;
};

class FileOutputStream : public OutputStream {
public:
  FileOutputStream(int fd, bool take_fd_ownership)
    : fd_(fd), take_fd_ownership_(take_fd_ownership) {
  }
  
  virtual ~FileOutputStream();
  
  void Write(const void* buf, size_t count);
  void Flush() {}
  
private:
  int fd_;
  bool take_fd_ownership_;
};

}