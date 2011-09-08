
#include "base/common.h"

#include <stddef.h>

namespace cheaproute {
  class JsonWriter;
  class JsonReader;
  
  void SerializePacket(JsonWriter* writer, const void* packet, size_t size);
  
  // Deserializes a single packet from the JsonReader. The reader is expected
  // to be initially be pointing at the JSON_StartObject token at the top of
  // packet, and will be pointing at the JSON_EndObject token at the bottom
  // of the packet after it has returned successfully. If an error occurs,
  // the function will return false and out_err will contain a detailed
  // description of the error
  bool DeserializePacket(JsonReader* reader, vector<uint8_t>* dest_buffer, string* out_err);
}