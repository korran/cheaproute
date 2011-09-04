
#include "test_util/stream.h"
#include "gtest/gtest.h"

namespace cheaproute {

FakeInputStream::FakeInputStream(size_t max_read_size, const char* data)
    : memory_stream_(data, strlen(data)),
      max_read_size_(max_read_size) {
  
}
FakeInputStream::FakeInputStream(size_t max_read_size, const void* data, size_t size)
    : memory_stream_(data, size),
      max_read_size_(max_read_size) {
}
    
  
void AssertStreamContents(const string& expected_contents,
                                const MemoryOutputStream* stream) {
  string actual;
  if (stream->size() > 0) {
    actual = string(static_cast<const char*>(stream->ptr()), stream->size());
  }
  ASSERT_EQ(expected_contents, actual);
}

}