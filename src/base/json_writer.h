
#include "base/common.h"
#include "base/stream.h"

namespace cheaproute {

enum JsonWriterMode {
  JsonWriterMode_DocumentStart,
  JsonWriterMode_StartObject,
  JsonWriterMode_ObjectPropertyName,
  JsonWriterMode_ObjectPropertyValue,
  JsonWriterMode_StartArray,
  JsonWriterMode_MiddleArray
};
  
class JsonWriter {
public:
  JsonWriter(TransferredOwnershipPtr<BufferedOutputStream> stream);
  
  void WritePropertyName(const string& str) { WritePropertyName(str.c_str()); }
  void WritePropertyName(const char* str);
  void WriteString(const string& str) { WriteString(str.c_str()); }
  void WriteString(const char* str);
  
  void WriteInteger(int value);
  void WriteInteger(int64_t value);
  void WriteBoolean(bool value);
  void WriteNull();
  
  void BeginArray();
  void EndArray();
  
  void BeginObject();
  void EndObject();
  
  void Flush();
  
private:
  void WriteRawString(const char* str);
  void BeginValue();
  JsonWriterMode mode_;
  stack<JsonWriterMode> mode_stack_;
  scoped_ptr<BufferedOutputStream> stream_;
};
  
}