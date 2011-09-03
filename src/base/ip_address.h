
#include "base/common.h"

#include <string.h>

namespace cheaproute {
  
struct Ip4Address {
  unsigned char addr[4];
  
  Ip4Address() {
    addr[0] = 0;
    addr[1] = 0;
    addr[2] = 0;
    addr[3] = 0;
  }
  Ip4Address(uint32_t val) {
    addr[0] = static_cast<char>((val & 0xff000000) >> 24);
    addr[1] = static_cast<char>((val & 0x00ff0000) >> 16);
    addr[2] = static_cast<char>((val & 0x0000ff00) >> 8);
    addr[3] = static_cast<char>((val & 0x000000ff) >> 0);
  }
  Ip4Address(const void* ptr, size_t size);
  
  string ToString() const;
  
  bool operator==(const Ip4Address& other) const {
    return memcmp(addr, other.addr, sizeof(addr)) == 0;
  }
  bool operator!=(const Ip4Address& other) const {
    return !(*this == other);
  }
};

struct Ip6Address {
  unsigned char addr[16];
  
  Ip6Address() {
    memset(addr, 0, sizeof(addr));
  }
  Ip6Address(const void* ptr, size_t size);
  
  string ToString() const;
  bool operator==(const Ip4Address& other) const {
    return memcmp(addr, other.addr, sizeof(addr)) == 0;
  }
  bool operator!=(const Ip4Address& other) const {
    return !(*this == other);
  }
};

const Ip4Address kIp4Any;

struct Ip4AddressInfo {
  Ip4Address address;
  Ip4Address netmask;
  Ip4Address broadcast;
  
  string ToString() const;
};

}