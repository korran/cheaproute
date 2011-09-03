#pragma once

#include "base/stream.h"

namespace cheaproute {
  
  class FakeInputStream : public InputStream {
  public:
    FakeInputStream(size_t max_read_size, const char* data)
        : memory_stream_(data, strlen(data)),
          max_read_size_(max_read_size) {
      
    }
    FakeInputStream(size_t max_read_size, const void* data, size_t size)
        : memory_stream_(data, size),
          max_read_size_(max_read_size) {
    }
    
    virtual ssize_t Read(void* buf, size_t count) {
      return memory_stream_.Read(buf, std::min(count, max_read_size_));
    }
    
  private:
    MemoryInputStream memory_stream_;
    size_t max_read_size_;
  };
  
  inline TransferredOwnershipPtr<BufferedInputStream> CreateBufferedInputStream(const char* str) {
    return TransferOwnership(new BufferedInputStream(
      TransferOwnership<InputStream>(new FakeInputStream(1000000, str)), 4096));
  }
}
