
#include "base/common.h"

#include "base/stream.h"
#include <algorithm>

namespace cheaproute {

  
BufferedInputStream::BufferedInputStream(TransferredOwnershipPtr<InputStream> delegatee, size_t buffer_size) 
  : delegatee_(delegatee.Release()) {
  buffer_.resize(buffer_size);
  pos_ = buffer_.begin();
  end_ = buffer_.begin();
}

ssize_t BufferedInputStream::Read(void* buf, size_t count) {
  assert(pos_ <= buffer_.end());
  
  uint8_t* dest = static_cast<uint8_t*>(buf);
  
  size_t bytes_available = end_ - pos_;
  
  if (bytes_available > 0) {
    size_t bytes_to_copy = std::min(bytes_available, count);
    std::copy(pos_, pos_ + bytes_to_copy, dest);
    pos_ += bytes_to_copy;
    return bytes_to_copy;
  }
  
  // If there are many bytes to read, then we will skip the
  // buffering process and read directly in to the user's buffer
  if (count >= buffer_.size() / 2) {
    return delegatee_->Read(buf, count);
  }
  
  ssize_t read_result = FillBuffer();
  
  if (read_result > 0) {
    size_t bytes_to_copy = std::min(static_cast<size_t>(read_result), count);
    std::copy(pos_, pos_ + bytes_to_copy, dest);
    pos_ += bytes_to_copy;
    return bytes_to_copy;
  }
  
  return read_result;
  
}

ssize_t BufferedInputStream::FillBuffer() {
  ssize_t bytes_read = delegatee_->Read(&buffer_[0], buffer_.size());
  assert(bytes_read <= static_cast<ssize_t>(buffer_.size()));
  pos_ = buffer_.begin();
  end_ = pos_ + std::max(static_cast<ssize_t>(0), bytes_read);
  return bytes_read;
}

int BufferedInputStream::PeekSlowPath() {
  if (FillBuffer() > 0)
    return *pos_;
  else
    return -1;
}

int BufferedInputStream::ReadSlowPath() {
  if (FillBuffer() > 0)
    return *(pos_++);
  else
    return -1;
}

}