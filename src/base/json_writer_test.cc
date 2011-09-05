
#include "base/stream.h"

#include "gtest/gtest.h"
#include "test_util/stream.h"
#include "test_util/json.h"
#include "base/json_reader.h"
#include "json_writer.h"
#include "test_util/json.h"

#include <stdint.h>

namespace cheaproute {

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

TEST(JsonWriter, Indent) {
  JsonWriterFixture fixture(JsonWriterFlags_Indent);
  JsonWriter* writer = fixture.writer();
  writer->BeginObject();
  writer->WritePropertyName("foo");
  writer->WriteString("bar");
  writer->WritePropertyName("array");
  writer->BeginArray();
  writer->WriteInteger(0);
  writer->WriteInteger(1);
  writer->WriteInteger(2);
  writer->EndArray();
  writer->WritePropertyName("object");
  writer->BeginObject();
  writer->WritePropertyName("a");
  writer->WriteString("b");
  writer->EndObject();
  writer->EndObject(); 
  
  fixture.AssertContents("{\n"
                         "  \"foo\": \"bar\",\n"
                         "  \"array\": [\n"
                         "    0,\n"
                         "    1,\n"
                         "    2\n"
                         "  ],\n"
                         "  \"object\": {\n"
                         "    \"a\": \"b\"\n"
                         "  }\n"
                         "}");
}

TEST(JsonWriter, IndentWithPackedLines) {
  JsonWriterFixture fixture(JsonWriterFlags_Indent);
  JsonWriter* writer = fixture.writer();
  writer->BeginObject();
  writer->WritePropertyName("foo");
  writer->WriteString("bar");
  writer->WritePropertyName("array");
  
  writer->BeginPack();
  writer->BeginArray();
  writer->WriteInteger(0);
  writer->WriteInteger(1);
  writer->WriteInteger(2);
  writer->EndArray();
  writer->EndPack();
  
  writer->WritePropertyName("object");
  
  writer->BeginPack();
  writer->BeginObject();
  writer->WritePropertyName("a");
  writer->WriteString("b");
  writer->EndObject();
  writer->EndPack();
  
  writer->WritePropertyName("x");
  writer->BeginPack();
  writer->WriteString("X");
  writer->WritePropertyName("y");
  writer->WriteString("Y");
  writer->WritePropertyName("z");
  writer->WriteString("Z");
  writer->EndPack();
  
  writer->EndObject(); 
  
  fixture.AssertContents("{\n"
                         "  \"foo\": \"bar\",\n"
                         "  \"array\": [0, 1, 2],\n"
                         "  \"object\": {\"a\": \"b\"},\n"
                         "  \"x\": \"X\", \"y\": \"Y\", \"z\": \"Z\"\n"
                         "}");
}


}