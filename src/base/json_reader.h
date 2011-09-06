
#pragma once

// I really hate to reinvent the wheel here, but I can't seem to find a simple C++ library
// for doing streaming pull-based JSON parsing (kinda like STAX)
#include "base/common.h"
#include "base/stream.h"

namespace cheaproute {
  
enum JsonToken {
  JSON_None,
  JSON_StartArray,
  JSON_EndArray,
  JSON_StartObject,
  JSON_EndObject,
  JSON_PropertyName,
  JSON_String,
  JSON_Integer,
  JSON_Float,
  JSON_Boolean,
  JSON_Null
};

const char* GetJsonTokenName(JsonToken token);

enum JsonError {
  JsonError_None,
  JsonError_InvalidUnicodeEscapeSequence,
  JsonError_InvalidEscapeSequence,
  JsonError_UnexpectedCharacter,
  JsonError_OutOfRange
};

enum JsonMode {
  JsonMode_DocumentStart,
  JsonMode_Array,
  JsonMode_Object
};


class JsonReader {
public:
  JsonReader(TransferredOwnershipPtr<BufferedInputStream> stream);
  
  JsonToken token_type() const { return token_type_; }
  const string& str_value() const { return str_value_; }
  int64_t int_value() const { return int_value_; }
  double float_value() const { return double_value_; }
  JsonError error_code() const { return error_code_; }
  bool bool_value() const { return bool_value_; }
  bool Next();
  
private:
  bool SkipWhitespace(int* out_initial_char);
  bool ReadValue(int initial_char);
  bool ReadNumber(int initial_char);
  bool ReadString(int initial_char);
  bool ReadKeyword(int initial_char, const char* keyword);
  bool ReadUnicodeEscapeCharacter(int* out_char);
  bool PopMode();
  
  scoped_ptr<BufferedInputStream> stream_;
  JsonError error_code_;
  JsonToken token_type_;
  JsonMode mode_;
  string str_value_;
  int64_t int_value_;
  double double_value_;
  bool bool_value_;
  stack<JsonMode> mode_stack_;
};

}