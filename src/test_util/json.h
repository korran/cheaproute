#pragma once

#include "test_util/stream.h"
#include "base/json_writer.h"

namespace cheaproute {

class JsonWriterFixture {
public:
  JsonWriterFixture(JsonWriterFlags flags) {
    Init(flags);
  }
  JsonWriterFixture() {
    Init(JsonWriterFlags_None);
  }
  void AssertContents(const string& expected_json) {
    writer_->Flush();
    AssertStreamContents(expected_json, mem_output_stream_);
  }
  
  JsonWriter* writer() { return writer_.get(); }
  
private:
  void Init(JsonWriterFlags flags) {
    mem_output_stream_ = new MemoryOutputStream();
    
    writer_.reset(new JsonWriter(shared_ptr<BufferedOutputStream>(
      new BufferedOutputStream(shared_ptr<OutputStream>(mem_output_stream_),
                               4096)), flags));
  }
  MemoryOutputStream* mem_output_stream_;
  scoped_ptr<JsonWriter> writer_;
};

}