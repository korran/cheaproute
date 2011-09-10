
#include "base/common.h"
#include "base/json_reader.h"

#include <errno.h>
#include <stdlib.h>

namespace cheaproute {
  
  
const char* kJsonTokenNames[] = {
  "None", "StartArray", "EndArray", "StartObject", "EndObject",
  "PropertyName", "String", "Integer", "Float", "Boolean", "Null"
};

const char* GetJsonTokenName(JsonToken token) {
  size_t i_token = static_cast<size_t>(token);
  if (i_token < ArrayLength(kJsonTokenNames))
    return kJsonTokenNames[i_token];
  return "Unknown";
}

// It might be better to use glibc's iconv functions
// for this, but they seem a bit too heavyweight for such
// a simple operation
static void AppendCharacterAsUtf8(string* dest, int ch) {
  if (ch >= 0) {
    if (ch <= 0x7F) {
      dest->push_back(static_cast<char>(ch));
    } else if (ch <= 0x07FF) {
      dest->push_back(static_cast<char>(0xC0 | ((ch & 0x000007C0) >> 6)));
      dest->push_back(static_cast<char>(0x80 | ((ch & 0x0000003f) >> 0)));
    } else if (ch <= 0xFFFF) {
      dest->push_back(static_cast<char>(0xE0 | ((ch & 0x0000F000) >> 12)));
      dest->push_back(static_cast<char>(0x80 | ((ch & 0x00000FC0) >> 6)));
      dest->push_back(static_cast<char>(0x80 | ((ch & 0x0000003f) >> 0)));
    } else if (ch <= 0x1FFFFF) {
      dest->push_back(static_cast<char>(0xF0 | ((ch & 0x001C0000) >> 18)));
      dest->push_back(static_cast<char>(0x80 | ((ch & 0x0003F000) >> 12)));
      dest->push_back(static_cast<char>(0x80 | ((ch & 0x00000FC0) >> 6)));
      dest->push_back(static_cast<char>(0x80 | ((ch & 0x0000003f) >> 0)));
    }
  }
}

JsonReader::JsonReader(shared_ptr<BufferedInputStream> stream)
  : stream_(stream),
    error_code_(JsonError_None),
    token_type_(JSON_None),
    mode_(JsonMode_DocumentStart),
    int_value_(0),
    double_value_(0.0),
    bool_value_(false) {
}

bool JsonReader::ReadValue(int initial_char) {
  switch (initial_char)
  {
    case '{':
      mode_stack_.push(mode_);
      token_type_ = JSON_StartObject;
      mode_ = JsonMode_Object;
      return true;
      
    case '[':
      mode_stack_.push(mode_);
      token_type_ = JSON_StartArray;
      mode_ = JsonMode_Array;
      return true;
      
    case '"':
    case '\'':
      token_type_ = JSON_String;
      return ReadString(initial_char);
      
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return ReadNumber(initial_char);
      
    case 'n':
      token_type_ = JSON_Null;
      return ReadKeyword(initial_char, "null");
      
    case 't':
      token_type_ = JSON_Boolean;
      bool_value_ = true;
      return ReadKeyword(initial_char, "true");
      
    case 'f':
      token_type_ = JSON_Boolean;
      bool_value_ = false;
      return ReadKeyword(initial_char, "false");
      
    default:
      error_code_ = JsonError_UnexpectedCharacter;
      return false;
  }
}

bool JsonReader::ReadKeyword(int initial_char, const char* keyword) {
  int ch = initial_char;
  
  for (const char* p = keyword ; *p != '\0'; p++) {
    if (p != keyword)
      stream_->Read();
    
    if (ch == -1)
      return false;
    if (ch != *p) {
      error_code_ = JsonError_UnexpectedCharacter;
      return false;
    }
    ch = stream_->Peek();
  }
  return true;
}

bool JsonReader::ReadNumber(int initial_char) {
  str_value_.clear();
  
  bool seen_dot = false;
  bool seen_exponent = false;
  bool seen_initial_zero = false;
  
  int ch = initial_char;
  if (ch == '-') {
    str_value_.push_back('-');
    ch = stream_->Read();
  }
  for (int i = 0;; i++) {
    switch (ch) {
      case -1:
      case '}':
      case ']':
      case ',':
      case '\r':
      case '\n':
      case '\t':
      case ' ':
      {
        if (str_value_.size() == 0) {
          error_code_ = JsonError_UnexpectedCharacter;
          return false;
        }
        if (seen_exponent || seen_dot) {
          token_type_ = JSON_Float;
          errno = 0;
          double_value_ = strtod(str_value_.c_str(), NULL);
          if (errno) {
            error_code_ = JsonError_OutOfRange;
            return false;
          }
        }
        else {
          token_type_ = JSON_Integer;
          errno = 0;
          int_value_ = strtoll(str_value_.c_str(), NULL, 10);
          if (errno) {
            error_code_ = JsonError_OutOfRange;
            return false;
          }
        }
        
        return true;
        break;
      }
      
      case '.': {
        if (seen_dot || seen_exponent) {
          error_code_ = JsonError_UnexpectedCharacter;
          return false;
        }
        seen_dot = true;
        str_value_.push_back((char) ch);
        break;
      }
      
      case 'e':
      case 'E': {
        if (seen_exponent) {
          
        }
        seen_exponent = true;
        str_value_.push_back((char) ch);
        break;
      }
      
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        if (ch == '0' && i == 0) {
          seen_initial_zero = true;
        } else if (seen_initial_zero && !seen_dot) {
          error_code_ = JsonError_UnexpectedCharacter;
          return false;
        }
        str_value_.push_back(static_cast<char>(ch));
        break;
      }
    }
    if (i > 0) {
      stream_->Read();
    }
    ch = stream_->Peek();
  }
  return true;
}

bool JsonReader::ReadUnicodeEscapeCharacter(int* out_char) {
  int ch;
  int value = 0;
  for (int i = 0; i < 4; i++) {
    ch = stream_->Read();
    if (ch == -1)
      return false;
    int digit = kHexValues[static_cast<uint8_t>(ch & 0xff)];
    if (digit == -1) {
      error_code_ = JsonError_InvalidUnicodeEscapeSequence;
      return false;
    }
    value = (value << 4) | digit;
  }
  *out_char = value;
  return true;
}

bool JsonReader::ReadString(int initial_char) {
  str_value_.clear();
  
  if (initial_char != '\"' && initial_char != '\'') {
    error_code_ = JsonError_UnexpectedCharacter;
    return false;
  }
  
  while (true) {
    int ch = stream_->Read();
    if (ch == -1)
      return false;
    if (ch == initial_char)
      return true;
    if (ch == '\\') {
      ch = stream_->Read();
      switch (ch) {
        case -1: {
          error_code_ = JsonError_UnexpectedCharacter;
          return false;
        }
        case 'u': {
          int value = 0;
          if (!ReadUnicodeEscapeCharacter(&value))
            return false;
          AppendCharacterAsUtf8(&str_value_, value);
          break;
        }
        case '"':
        case '/':
        case '\\': {
          str_value_.push_back((char)ch);
          break;
        }
        case 'b': {
          str_value_.push_back('\b');
          break;
        }
        case 'f': {
          str_value_.push_back('\f');
          break;
        }
        case 'n': {
          str_value_.push_back('\n');
          break;
        }
        case 'r': {
          str_value_.push_back('\r');
          break;
        }
        case 't': {
          str_value_.push_back('\t');
          break;
        }
        
        default: {
          error_code_ = JsonError_InvalidEscapeSequence;
          return false;
        }
      }
      continue;
    }
    str_value_.push_back((char)ch);
  }
}

bool JsonReader::SkipWhitespace(int* out_initial_char) {
  int ch = ' ';
  while (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') {
    ch = stream_->Read();
    if (ch == -1)
      return false;
  }
  *out_initial_char = ch;
  return true;
}

bool JsonReader::PopMode() {
  if (mode_stack_.empty()) {
    error_code_ = JsonError_UnexpectedCharacter;
    return false;
  }
  mode_ = mode_stack_.top();
  mode_stack_.pop();
  return true;
}

bool JsonReader::Next() {
  int ch;
  if (!SkipWhitespace(&ch)) 
    return false;

  switch (mode_)
  {
    case JsonMode_DocumentStart: {
      return ReadValue(ch);
    }
    case JsonMode_Array: {
      if (ch == ']') {
        token_type_ = JSON_EndArray;
        return PopMode();
      }
      if (token_type_ != JSON_StartArray) {
        if (ch != ',') {
          error_code_ = JsonError_UnexpectedCharacter;
          return false;
        }
        if (!SkipWhitespace(&ch))
          return false;
      }
      return ReadValue(ch);
    }
    
    case JsonMode_Object: {
      if (token_type_ != JSON_PropertyName) {
        if (ch == '}') {
          token_type_ = JSON_EndObject;
          return PopMode();
        }
        if (token_type_ != JSON_StartObject) {
          if (ch != ',') {
            error_code_ = JsonError_UnexpectedCharacter;
            return false;
          }
          if (!SkipWhitespace(&ch)) 
            return false;
        }
        token_type_ = JSON_PropertyName;
        if (!ReadString(ch))
          return false;
        if (!SkipWhitespace(&ch))
          return false;
        
        if (ch != ':') {
          error_code_ = JsonError_UnexpectedCharacter;
          return false;
        }
        return true;
      } else {
        return ReadValue(ch);
      }
    }
  }
    
  return false;
}


}
