#pragma once

#include "base/stream.h"

namespace cheaproute {
  
  class FakeInputStream : public InputStream {
  public:
    FakeInputStream(size_t max_read_size, const char* data);
    FakeInputStream(size_t max_read_size, const void* data, size_t size);
    
    virtual ssize_t Read(void* buf, size_t count) {
      return memory_stream_.Read(buf, std::min(count, max_read_size_));
    }
    
  private:
    MemoryInputStream memory_stream_;
    size_t max_read_size_;
  };
  
  inline TransferredOwnershipPtr<BufferedInputStream> 
              CreateBufferedInputStream(const char* str) {
    return TransferOwnership(new BufferedInputStream(
      TransferOwnership<InputStream>(new FakeInputStream(1000000, str)), 4096));
  }
  void AssertStreamContents(const string& expected_contents,
                                const MemoryOutputStream* stream);
}
