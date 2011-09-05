
#include <stddef.h>

namespace cheaproute {
  class JsonWriter;
  
  void SerializePacket(JsonWriter* writer, const void* packet, size_t size);
}