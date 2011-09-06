
#include "base/common.h"

#include <stddef.h>

namespace cheaproute {
  class JsonWriter;
  class JsonReader;
  
  void SerializePacket(JsonWriter* writer, const void* packet, size_t size);
  bool DeserializePacket(JsonReader* reader, vector<uint8_t>* dest_buffer, string* out_err);
}