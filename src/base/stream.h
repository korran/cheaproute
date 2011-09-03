#pragma once

#include "base/common.h"

namespace cheaproute {
  
class InputStream {
public:
  virtual ~InputStream() {}
  virtual ssize_t Read(void* buf, size_t count) = 0;
};


// This class is preferable to FILE* because character-by-character
// reading (using Read()) can be inlined by the compiler
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

}