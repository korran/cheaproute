
#include "base/common.h"

#include <string.h>

namespace cheaproute {
  
struct Ip4Address {
  uint8_t addr[4];
  
  Ip4Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    addr[0] = a;
    addr[1] = b;
    addr[2] = c;
    addr[3] = d;
  }
  
  Ip4Address() {
    addr[0] = 0;
    addr[1] = 0;
    addr[2] = 0;
    addr[3] = 0;
  }
  Ip4Address(uint32_t val);
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
  uint8_t addr[16];
  
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
inline bool operator<(const Ip6Address& a, const Ip6Address b) {
  return memcmp(&a.addr, &b.addr, sizeof(a.addr)) < 0;
}

const Ip4Address kIp4Any;

struct Ip4AddressInfo {
  Ip4AddressInfo()
    : prefix_len(0) {
  }
  
  Ip4AddressInfo(Ip4Address address, Ip4Address broadcast, uint8_t prefix_len)
    : address(address), 
      broadcast(broadcast),
      prefix_len(prefix_len) {      
  }
  
  Ip4Address address;
  Ip4Address broadcast;
  uint8_t prefix_len;
  
  string ToString() const;
};


inline bool operator<(const Ip4AddressInfo& a, const Ip4AddressInfo b) {
  return memcmp(&a.address, &b.address, sizeof(a.address)) < 0;
}
}