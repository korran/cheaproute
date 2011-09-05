
#include "base/common.h"

#include "base/stream.h"
#include "base/file_descriptor.h"

#include <algorithm>
#include <string.h>

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

BufferedOutputStream::BufferedOutputStream(TransferredOwnershipPtr<OutputStream> delegatee, size_t buffer_size)
    : delegatee_(delegatee.Release()) {
  buffer_.resize(buffer_size);
  pos_ = buffer_.begin();
  end_ = buffer_.end();
}

void BufferedOutputStream::Write(const void* buf, size_t size) {
  if (size >= buffer_.size() / 2) {
    Flush();
    delegatee_->Write(buf, size);
  } else {
    const uint8_t* src = static_cast<const uint8_t*>(buf);
    const uint8_t* src_end = src + size;
    while (src < src_end) {
      size_t write_size = std::min(src_end - src, end_ - pos_);
      pos_ = std::copy(src, src + write_size, pos_);
      src += write_size;
      if (end_ == pos_) {
        Flush();
      }
    }
  }
  
}

void BufferedOutputStream::Flush() {
  size_t num_bytes_to_flush = pos_ - buffer_.begin();
  if (num_bytes_to_flush > 0) {
    delegatee_->Write(&buffer_[0], num_bytes_to_flush);
    pos_ = buffer_.begin();
  }
}

MemoryInputStream::MemoryInputStream(const void* data, size_t size)
  : pos_(0) {
  data_.resize(size);
  memcpy(&data_[0], data, data_.size());
}

ssize_t MemoryInputStream::Read(void* buf, size_t count) {
  size_t copy_size = std::min(count, data_.size() - pos_);
  memcpy(buf, &data_[pos_], copy_size);
  pos_ += copy_size;
  return copy_size;
}

MemoryOutputStream::MemoryOutputStream() 
    : pos_(0) {
  buffer_.resize(16);
} 

void MemoryOutputStream::Write(const void* buf, size_t count) {
  if (count > buffer_.size()) {
    buffer_.resize(std::max(buffer_.size() * 2, count));
  }
  memcpy(&buffer_[pos_], buf, count);
  pos_ += count;
}

FileOutputStream::~FileOutputStream() {
  if (take_fd_ownership_) {
    close(fd_);
  }
}

void FileOutputStream::Write(const void* buf, size_t count) {
  size_t total_bytes_written = 0;
  while (total_bytes_written < count) {
    size_t bytes_written = CheckFdOp(write(fd_, buf, count), "writing to fd");
    if (bytes_written == 0) {
      AbortWithMessage("write() returned 0");
    }
    total_bytes_written += bytes_written;
    
  }
}

}