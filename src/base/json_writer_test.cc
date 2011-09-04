
#include "base/stream.h"

#include "gtest/gtest.h"
#include "test_util/stream.h"
#include "base/json_reader.h"
#include "json_writer.h"

#include <stdint.h>

namespace cheaproute {

class JsonWriterFixture {
public:
  JsonWriterFixture() {
    mem_output_stream_ = new MemoryOutputStream();
    
    writer_.reset(new JsonWriter(TransferOwnership(new BufferedOutputStream(
      TransferOwnership<OutputStream>(mem_output_stream_), 4096))));
  }
  void AssertContents(const string& expected_json) {
    writer_->Flush();
    AssertStreamContents(expected_json, mem_output_stream_);
  }
  
  JsonWriter* writer() { return writer_.get(); }
  
private:
  MemoryOutputStream* mem_output_stream_;
  scoped_ptr<JsonWriter> writer_;
};

void TestJsonWriteString(const string& expected_json, const string& value) {
  JsonWriterFixture fixture;
  fixture.writer()->WriteString(value);
  fixture.AssertContents(expected_json);
}
void TestJsonWriteBoolean(const string& expected_json, bool value) {
  JsonWriterFixture fixture;
  fixture.writer()->WriteBoolean(value);
  fixture.AssertContents(expected_json);
}
void TestJsonWriteInteger(const string& expected_json, int value) {
  JsonWriterFixture fixture;
  fixture.writer()->WriteInteger(value);
  fixture.AssertContents(expected_json);
}
void TestJsonWriteInteger(const string& expected_json, int64_t value) {
  JsonWriterFixture fixture;
  fixture.writer()->WriteInteger(value);
  fixture.AssertContents(expected_json);
}

TEST(JsonWriter, String) {
  TestJsonWriteString("\"hello\"", "hello");
  TestJsonWriteString("\"\\r\\n\\t\\f\\b\\\\\\\"\\u0001\\u001f\"", 
                      "\r\n\t\f\b\\\"\x01\x1f");
}

TEST(JsonWriter, Integer) {
  TestJsonWriteInteger("0", 0);
  TestJsonWriteInteger("12345", 12345);
  TestJsonWriteInteger("-12345", -12345);
  TestJsonWriteInteger("2147483647", 2147483647);
  TestJsonWriteInteger("-2147483648", -2147483648);
  TestJsonWriteInteger("12734987234838", static_cast<int64_t>(12734987234838LL));
  TestJsonWriteInteger("9223372036854775807", INT64_MAX);
  TestJsonWriteInteger("-9223372036854775808", INT64_MIN);
}

TEST(JsonWriter, Boolean) {
  TestJsonWriteBoolean("true", true);
  TestJsonWriteBoolean("false", false);
}

TEST(JsonWriter, Object) {
  JsonWriterFixture fixture;
  JsonWriter* writer = fixture.writer();
  
  writer->BeginObject();
  writer->WritePropertyName("str");
  writer->WriteString("val");
  writer->WritePropertyName("int32");
  writer->WriteInteger(32);
  writer->WritePropertyName("int64");
  writer->WriteInteger(static_cast<int64_t>(10000000000000LL));
  writer->WritePropertyName("bool");
  writer->WriteBoolean(true);
  writer->WritePropertyName("null");
  writer->WriteNull();
  
  writer->WritePropertyName("emptyObject");
  writer->BeginObject();
  writer->EndObject();
  
  writer->WritePropertyName("onePropObject");
  writer->BeginObject();
  writer->WritePropertyName("foo");
  writer->WriteString("bar");
  writer->EndObject();
  
  writer->WritePropertyName("twoPropObject");
  writer->BeginObject();
  writer->WritePropertyName("a");
  writer->WriteString("A");
  writer->WritePropertyName("b");
  writer->WriteString("B");
  writer->EndObject();
  
  writer->WritePropertyName("emptyArray");
  writer->BeginArray();
  writer->EndArray();
  
  writer->WritePropertyName("arrayOf1");
  writer->BeginArray();
  writer->WriteInteger(0);
  writer->EndArray();
  
  writer->WritePropertyName("arrayOf2");
  writer->BeginArray();
  writer->WriteInteger(0);
  writer->WriteInteger(1);
  writer->EndArray();
  
  writer->EndObject();
  
  fixture.AssertContents("{"
                           "\"str\":\"val\","
                           "\"int32\":32,"
                           "\"int64\":10000000000000,"
                           "\"bool\":true,"
                           "\"null\":null,"
                           "\"emptyObject\":{},"
                           "\"onePropObject\":{\"foo\":\"bar\"},"
                           "\"twoPropObject\":{\"a\":\"A\",\"b\":\"B\"},"
                           "\"emptyArray\":[],"
                           "\"arrayOf1\":[0],"
                           "\"arrayOf2\":[0,1]"
                         "}");
}

TEST(JsonWriter, Array) {
  JsonWriterFixture fixture;
  JsonWriter* writer = fixture.writer();
  
  writer->BeginArray();
  writer->WriteString("val");
  writer->WriteInteger(32);
  writer->WriteInteger(static_cast<int64_t>(-10000000000000));
  writer->WriteBoolean(false);
  writer->WriteNull();
  writer->BeginObject();
  writer->EndObject();
  
  writer->BeginObject();
  writer->WritePropertyName("foo");
  writer->WriteString("bar");
  writer->EndObject();
  
  writer->BeginObject();
  writer->WritePropertyName("a");
  writer->WriteString("A");
  writer->WritePropertyName("b");
  writer->WriteString("B");
  writer->EndObject();
  
  writer->BeginArray();
  writer->EndArray();
  
  writer->BeginArray();
  writer->WriteInteger(0);
  writer->EndArray();
  
  writer->BeginArray();
  writer->WriteInteger(0);
  writer->WriteInteger(1);
  writer->EndArray();
  
  writer->EndArray();
  
  fixture.AssertContents("["
                           "\"val\","
                           "32,"
                           "-10000000000000,"
                           "false,"
                           "null,"
                           "{},"
                           "{\"foo\":\"bar\"},"
                           "{\"a\":\"A\",\"b\":\"B\"},"
                           "[],"
                           "[0],"
                           "[0,1]"
                         "]");
}
  
}