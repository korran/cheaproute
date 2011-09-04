
#include "base/json_writer.h"

#include <inttypes.h>
#include <string.h>

namespace cheaproute {
  
JsonWriter::JsonWriter(TransferredOwnershipPtr<BufferedOutputStream> stream)
  : mode_(JsonWriterMode_DocumentStart) {
  stream_.reset(stream.Release());
}

void JsonWriter::Flush() {
  stream_->Flush();
}
  
const char kHexChars[] = {'0', '1', '2', '3', '4', '5', '6', '7', 
                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

const char kSpecialEscapes[] = { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                                'b','t','n', 0 ,'f','r', 0,  0 ,
                                 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                                 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 };

void JsonWriter::WritePropertyName(const char* str) {
  assert(mode_ == JsonWriterMode_StartObject || mode_ == JsonWriterMode_ObjectPropertyName);
  if (mode_ == JsonWriterMode_ObjectPropertyName) {
    stream_->Write(',');
  }
  mode_ = JsonWriterMode_ObjectPropertyValue;
  WriteRawString(str);
  stream_->Write(':');
}
                                 
void JsonWriter::WriteString(const char* str) {
  BeginValue();
  WriteRawString(str);
}

void JsonWriter::WriteRawString(const char* str) {
  const char* p = str;
  stream_->Write('"');
  while (uint8_t ch = *p++) {
    if (ch == '"' || ch == '\\') {
      stream_->Write('\\');
      stream_->Write(ch);
    } else if (ch < ' ') {
      char special_escape_code = kSpecialEscapes[ch];
      stream_->Write('\\');
      if (special_escape_code) {
        stream_->Write(special_escape_code);
      } else {
        stream_->Write('u');
        stream_->Write('0');
        stream_->Write('0');
        stream_->Write(kHexChars[(ch & 0xf0) >> 4]);
        stream_->Write(kHexChars[(ch & 0x0f) >> 0]);
      }
    } else {
      stream_->Write(ch);
    }
  }
  stream_->Write('"');
}

void JsonWriter::WriteInteger(int value) {
  BeginValue();
  
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%d", value);
  stream_->Write(buffer, strlen(buffer));
}
void JsonWriter::WriteInteger(int64_t value) {
  BeginValue();
  
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%"PRId64, value);
  stream_->Write(buffer, strlen(buffer));
}
void JsonWriter::WriteBoolean(bool value) {
  BeginValue();
  
  if (value) {
    stream_->Write("true", 4);
  } else {
    stream_->Write("false", 5);
  } 
}
void JsonWriter::WriteNull() {
  BeginValue();
  
  stream_->Write("null", 4);
}

void JsonWriter::BeginValue() {
  assert(mode_ != JsonWriterMode_ObjectPropertyName);
  if (mode_ == JsonWriterMode_ObjectPropertyValue) {
    mode_ = JsonWriterMode_ObjectPropertyName;
  }
  if (mode_ == JsonWriterMode_MiddleArray) {
    stream_->Write(',');
  }
  if (mode_ == JsonWriterMode_StartArray) {
    mode_ = JsonWriterMode_MiddleArray;
  }
}
void JsonWriter::BeginArray() {
  BeginValue();
  stream_->Write('[');
  mode_stack_.push(mode_);
  mode_ = JsonWriterMode_StartArray;
}
void JsonWriter::EndArray() {
  assert(mode_ == JsonWriterMode_StartArray || mode_ == JsonWriterMode_MiddleArray);
  stream_->Write(']');
  mode_ = mode_stack_.top();
  mode_stack_.pop();
}
void JsonWriter::BeginObject() {
  BeginValue();
  stream_->Write('{');
  mode_stack_.push(mode_);
  mode_ = JsonWriterMode_StartObject;
}
void JsonWriter::EndObject() {
  assert(mode_ == JsonWriterMode_ObjectPropertyName || mode_ == JsonWriterMode_StartObject);
  stream_->Write('}');
  mode_ = mode_stack_.top();
  mode_stack_.pop();
  
}

}