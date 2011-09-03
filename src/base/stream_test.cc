
#include "stream.h"

#include "gtest/gtest.h"

namespace cheaproute {


class MemoryInputStream : public InputStream {
public:
  MemoryInputStream(const void* data, size_t size)
    : pos_(0) {
    data_.resize(size);
    memcpy(&data_[0], data, data_.size());
  }
  
  virtual ssize_t Read(void* buf, size_t count) {
    size_t copy_size = std::min(count, data_.size() - pos_);
    memcpy(buf, &data_[pos_], copy_size);
    pos_ += copy_size;
    return copy_size;
  }
  
private:
  vector<uint8_t> data_;
  size_t pos_;
};
  
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

static void TestCharacterByCharacter(size_t max_read_size, size_t buffer_size) {
  BufferedInputStream stream(TransferOwnership<InputStream>(
    new FakeInputStream(max_read_size, "Hello World!")), buffer_size);
  
  ASSERT_EQ('H', stream.Peek());
  ASSERT_EQ('H', stream.Read());
  ASSERT_EQ('e', stream.Read());
  ASSERT_EQ('l', stream.Read());
  ASSERT_EQ('l', stream.Read());
  ASSERT_EQ('o', stream.Read());
  ASSERT_EQ(' ', stream.Read());
  ASSERT_EQ('W', stream.Peek());
  ASSERT_EQ('W', stream.Peek());
  ASSERT_EQ('W', stream.Read());
  ASSERT_EQ('o', stream.Read());
  ASSERT_EQ('r', stream.Read());
  ASSERT_EQ('l', stream.Read());
  ASSERT_EQ('d', stream.Read());
  ASSERT_EQ('!', stream.Read());
  ASSERT_EQ(-1, stream.Read());
  ASSERT_EQ(-1, stream.Read());
  ASSERT_EQ(-1, stream.Read());
  ASSERT_EQ(-1, stream.Peek());
}

TEST(BufferedInputStreamTest, CharacterByCharacterReading) {
  TestCharacterByCharacter(100, 4);
  TestCharacterByCharacter(6, 4);
  TestCharacterByCharacter(3, 4);
  TestCharacterByCharacter(2, 4);
  TestCharacterByCharacter(1, 4);
  TestCharacterByCharacter(2, 100);
  TestCharacterByCharacter(100, 100);
}

static string ReadString(InputStream* stream, size_t size) {
  vector<char> buffer;
  buffer.resize(size);
  size_t bytes_read = stream->Read(&buffer[0], size);
  assert(bytes_read >= 0);
  assert(bytes_read <= size);
  return string(&buffer[0], bytes_read);
}

TEST(BufferedInputStreamTest, NormalReading) {
  BufferedInputStream stream(TransferOwnership<InputStream>(
    new FakeInputStream(6, "Hello World!")), 4);
  
  ASSERT_EQ("Hello ", ReadString(&stream, 6));
  ASSERT_EQ("W", ReadString(&stream, 1));
  ASSERT_EQ("orl", ReadString(&stream, 10));
  ASSERT_EQ("d!", ReadString(&stream, 10));
  ASSERT_EQ("", ReadString(&stream, 10));
}


}