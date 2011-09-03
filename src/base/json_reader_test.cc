
#include "base/stream.h"

#include "gtest/gtest.h"
#include "test_util/stream.h"
#include "base/json_reader.h"

namespace cheaproute {

static void ExpectToken(JsonToken expected_token, JsonReader* reader) {
  ASSERT_TRUE(reader->Next());
  ASSERT_EQ(expected_token, reader->token_type());
}
  
static void ExpectString(const string& expected_value, JsonReader* reader) {
  ExpectToken(JSON_String, reader);
  ASSERT_EQ(expected_value, reader->str_value());
}

static void ExpectInteger(int64_t expected_value, JsonReader* reader) {
  ExpectToken(JSON_Integer, reader);
  ASSERT_EQ(expected_value, reader->int_value());
}

static void ExpectBoolean(bool expected_value, JsonReader* reader) {
  ExpectToken(JSON_Boolean, reader);
  ASSERT_EQ(expected_value, reader->bool_value());
}

static void ExpectError(JsonError expected_error, const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  while (reader.Next()) {
    ASSERT_EQ(JsonError_None, reader.error_code());
  }
}

static void ExpectFloat(double expected_value, JsonReader* reader) {
  ExpectToken(JSON_Float, reader);
  ASSERT_EQ(expected_value, reader->float_value());
}

static void ExpectPropertyName(const string& expected_value, JsonReader* reader) {
  ExpectToken(JSON_PropertyName, reader);
  ASSERT_EQ(expected_value, reader->str_value());
}

static void ExpectEnd(JsonReader* reader) {
  ASSERT_FALSE(reader->Next());
}
  
static void TestStringParse(const char* expected_result, const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  ExpectString(expected_result, &reader);
}
  
TEST(JsonReaderTest, BasicString) {
  TestStringParse("", "\"\"");
  TestStringParse(" ", "\" \"");
  TestStringParse("a string", "\"a string\"");
  TestStringParse("a string", "  \"a string\"");
  TestStringParse("a string", "\"a string\"  ");
  TestStringParse("a string", "\n\"a string\"  ");
  TestStringParse("a \"string\"", "\"a \\\"string\\\"\"");
  TestStringParse("backslash: \\", "\"backslash: \\\\\"");
}

TEST(JsonReaderTest, StringError) {
  ExpectError(JsonError_UnexpectedCharacter, "\"");
  ExpectError(JsonError_UnexpectedCharacter, "\"\\");
  ExpectError(JsonError_UnexpectedCharacter, "\"asd");
  ExpectError(JsonError_InvalidUnicodeEscapeSequence, "\"\\u123X\"");
  ExpectError(JsonError_InvalidEscapeSequence, "\"\\v\"");
}
TEST(JsonReaderTest, SimpleEscapeSequences) {
  TestStringParse("\r\n\t\f\b\"\\/", "\"\\r\\n\\t\\f\\b\\\"\\\\\\/\"");
}
TEST(JsonReaderTest, UnicodeStringEscaping) {
  TestStringParse("¡hola!", "\"\\u00A1\\u0068\\u006F\\u006c\\u0061\\u0021\"");
  TestStringParse("こんばんは", "\"\\u3053\\u3093\\u3070\\u3093\\u306F\"");
}

TEST(JsonReaderTest, Utf8Passthrough) {
  TestStringParse("¡hola!", "\"¡hola!\"");
  TestStringParse("こんばんは", "\"こんばんは\"");
}
TEST(JsonReaderTest, IllegalUtf8Passthrough) {
  TestStringParse("\xFF\xFF", "\"\xFF\xFF\"");
  TestStringParse("\xC0\x30", "\"\xC0\x30\"");
}

static void TestIntegerParse(int64_t expected_result, const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  ExpectInteger(expected_result, &reader);
}
TEST(JsonReaderTest, Integer) {
  TestIntegerParse(0, "0");
  TestIntegerParse(1234515367890, "1234515367890");
  TestIntegerParse(-1, "-1");
  TestIntegerParse(-572938261722453, "-572938261722453");
}
TEST(JsonReaderTest, BadInteger) {
  ExpectError(JsonError_UnexpectedCharacter, "--1");
  ExpectError(JsonError_UnexpectedCharacter, "1..");
  ExpectError(JsonError_UnexpectedCharacter, "01");
  ExpectError(JsonError_OutOfRange, "1231827319827382739181232327391872388");
  ExpectError(JsonError_OutOfRange, "-1231827319827382739181232327391872388");
}

static void TestFloatParse(double expected_result, const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  ExpectFloat(expected_result, &reader);
}
TEST(JsonReaderTest, Float) {
  TestFloatParse(0.0, "0.0");
  TestFloatParse(0.0000000001, "0.0000000001");
  TestFloatParse(-123.125, "-123.125");
  TestFloatParse(72.75, "72.75");
  TestFloatParse(72.75E5, "72.75E5");
  TestFloatParse(-72.75e5, "-72.75e5");
  TestFloatParse(72e5, "72e5");
}
TEST(JsonReaderTest, BadFloat) {
  ExpectError(JsonError_UnexpectedCharacter, "1.000e00e00");
  ExpectError(JsonError_UnexpectedCharacter, "1.12f");
  ExpectError(JsonError_UnexpectedCharacter, "00.12");
  ExpectError(JsonError_UnexpectedCharacter, "5.12");
  ExpectError(JsonError_OutOfRange, "5e1230");
  ExpectError(JsonError_OutOfRange, "-5e1230");
}

TEST(JsonReaderTest, Null) {
  JsonReader reader(CreateBufferedInputStream("null"));
  ExpectToken(JSON_Null, &reader);
  ExpectEnd(&reader);
}
TEST(JsonReaderTest, True) {
  JsonReader reader(CreateBufferedInputStream("true"));
  ExpectBoolean(true, &reader);
  ExpectEnd(&reader);
}
TEST(JsonReaderTest, False) {
  JsonReader reader(CreateBufferedInputStream("false"));
  ExpectBoolean(false, &reader);
  ExpectEnd(&reader);
}

static void TestKeywordArray(const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  ExpectToken(JSON_StartArray, &reader);
  ExpectBoolean(true, &reader);
  ExpectBoolean(false, &reader);
  ExpectToken(JSON_Null, &reader);
  ExpectToken(JSON_EndArray, &reader);
  ExpectEnd(&reader);
}
TEST(JsonReaderTest, KeywordArray) {
  TestKeywordArray("[true,false,null]");
  TestKeywordArray(" [ true , false , null ] ");
  TestKeywordArray(" \r\n\t[ \r\n\ttrue \t\n\t, \r\n\tfalse \r\n\t,"
                   " \r\n\tnull ] ");
}

void TestEmptyArray(const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  ExpectToken(JSON_StartArray, &reader);
  ExpectToken(JSON_EndArray, &reader);
  ExpectEnd(&reader);
}

TEST(JsonReaderTest, EmptyArray) {
  TestEmptyArray("[]");
  TestEmptyArray(" [ ] ");
  TestEmptyArray("\r\n\t[\r\n\t]\t\r\n");
}

void TestNestedEmptyArray(const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  ExpectToken(JSON_StartArray, &reader);
  ExpectToken(JSON_StartArray, &reader);
  ExpectToken(JSON_StartArray, &reader);
  ExpectToken(JSON_EndArray, &reader);
  ExpectToken(JSON_EndArray, &reader);
  ExpectToken(JSON_EndArray, &reader);
  ExpectEnd(&reader);
}

TEST(JsonReaderTest, NestedArray) {
  TestNestedEmptyArray("[[[]]]");
  TestNestedEmptyArray(" [ [ [ ] ] ] ");
  TestNestedEmptyArray(" \r\n\t[ \r\n\t[ \r\n\t[ \r\n\t] \r\n\t] \r\n\t] ");
}

TEST(JsonReaderTest, ArrayWithSingleString) {
  JsonReader reader(CreateBufferedInputStream("[\"hello\"]"));
  ExpectToken(JSON_StartArray, &reader);
  ExpectString("hello", &reader);
  ExpectToken(JSON_EndArray, &reader);
  ExpectEnd(&reader);
}

static void TestArrayWithTwoStrings(const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  ExpectToken(JSON_StartArray, &reader);
  ExpectString("hello", &reader);
  ExpectString("world", &reader);
  ExpectToken(JSON_EndArray, &reader);
  ExpectEnd(&reader);
}

TEST(JsonReaderTest, ArrayWithTwoStrings) {
  TestArrayWithTwoStrings("[\"hello\",\"world\"]");
  TestArrayWithTwoStrings(" [ \"hello\", \"world\" ]  ");
  TestArrayWithTwoStrings(" \r\n\t[ \r\n\t\"hello\" \r\n\t, \r\n\t\"world\" \r\n\t] \r\n\t");
}

TEST(JsonReaderTest, ArrayWithThreeStrings) {
  JsonReader reader(CreateBufferedInputStream("[\"hello\",\"non-cruel\",\"world\"]"));
  ExpectToken(JSON_StartArray, &reader);
  ExpectString("hello", &reader);
  ExpectString("non-cruel", &reader);
  ExpectString("world", &reader);
  ExpectToken(JSON_EndArray, &reader);
  ExpectEnd(&reader);
}

static void TestEmptyObject(const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  ExpectToken(JSON_StartObject, &reader);
  ExpectToken(JSON_EndObject, &reader);
  ExpectEnd(&reader);
}

TEST(JsonReaderTest, EmptyObject) {
  TestEmptyObject("{}");
  TestEmptyObject(" { } ");
  TestEmptyObject("\r\n\t{\r\n\t}\t\r\n");
}

static void TestNestedObject(const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  ExpectToken(JSON_StartObject, &reader);
  ExpectPropertyName("nested1", &reader);
  ExpectToken(JSON_StartObject, &reader);
  ExpectPropertyName("nested2", &reader);
  ExpectToken(JSON_StartObject, &reader);
  ExpectToken(JSON_EndObject, &reader);
  ExpectToken(JSON_EndObject, &reader);
  ExpectToken(JSON_EndObject, &reader);
  ExpectEnd(&reader);
}

TEST(JsonReaderTest, TestNestedObject) {
  TestNestedObject("{\"nested1\":{\"nested2\":{}}}");
  TestNestedObject(" { \"nested1\" : { \"nested2\" : { } } } ");
  TestNestedObject(" \r\n\t{ \r\n\t\"nested1\" \r\n\t: \r\n\t{ \r\n\t\"n"
                   "ested2\" \r\n\t: \r\n\t{ \r\n\t} \r\n\t} \r\n\t} \r\n\t");
}

static void TestComplexObject(const char* json) {
  JsonReader reader(CreateBufferedInputStream(json));
  ExpectToken(JSON_StartObject, &reader);
  ExpectPropertyName("a", &reader);
  ExpectString("A", &reader);
  ExpectPropertyName("b", &reader);
  ExpectInteger(123, &reader);
  ExpectPropertyName("c", &reader);
  ExpectFloat(1.5, &reader);
  ExpectPropertyName("d", &reader);
  ExpectToken(JSON_StartArray, &reader);
  ExpectInteger(1, &reader);
  ExpectInteger(2, &reader);
  ExpectToken(JSON_StartObject, &reader);
  ExpectPropertyName("foo", &reader);
  ExpectString("bar", &reader);
  ExpectToken(JSON_EndObject, &reader);
  ExpectToken(JSON_EndArray, &reader);
  ExpectToken(JSON_EndObject, &reader);
  ExpectEnd(&reader);
}
TEST(JsonReaderTest, ComplexObject) {
  TestComplexObject("{\"a\":\"A\",\"b\":123,\"c\":1.5,\"d\":[1,2,"
                    "{\"foo\":\"bar\"}]}");
  TestComplexObject(" { \"a\" : \"A\" , \"b\" : 123 , \"c\" : 1.5, \"d\" : "
                    "[ 1 , 2 , { \"foo\" : \"bar\" } ] } ");
}

}