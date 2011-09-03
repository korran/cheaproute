
#include "base/common.h"

#include "net/ip_address.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip6.h>
#include <string.h>

namespace cheaproute {

Ip4Address::Ip4Address(uint32_t network_val) {
  uint32_t host_val = htonl(network_val);
  addr[0] = static_cast<char>((host_val & 0xff000000) >> 24);
  addr[1] = static_cast<char>((host_val & 0x00ff0000) >> 16);
  addr[2] = static_cast<char>((host_val & 0x0000ff00) >> 8);
  addr[3] = static_cast<char>((host_val & 0x000000ff) >> 0);
}
Ip4Address::Ip4Address(const void* ptr, size_t size) {
  assert(size == 4);
  memcpy(&addr[0], ptr, 4);
}

string Ip4Address::ToString() const {
  struct in_addr sock_addr;
  sock_addr.s_addr = htonl((addr[0] << 24) |
                           (addr[1] << 16) |
                           (addr[2] << 8) |
                           (addr[3] << 0));
  char buffer[32];
  
  CheckPosixOp(inet_ntop(AF_INET, &sock_addr, buffer, sizeof(buffer)),
               "formatting Ip4 address");
  
  return buffer;
}

Ip6Address::Ip6Address(const void* ptr, size_t size) {
  assert(size == 16);
  memcpy(&addr[0], ptr, 16);
}

string Ip6Address::ToString() const
{
  struct in6_addr sock_addr;
  memcpy(sock_addr.s6_addr, addr, 16);
  char buffer[64];
  
  CheckPosixOp(inet_ntop(AF_INET6, &sock_addr, buffer, sizeof(buffer)),
               "formatting Ip6 address");
  
  return buffer;
}

string Ip4AddressInfo::ToString() const
{
  StringPrinter ss;
  ss.Printf("[addr: %s", this->address.ToString().c_str());
  if (this->broadcast != kIp4Any) 
    ss.Printf(" bcast: %s", this->broadcast.ToString().c_str());
  if (this->netmask != kIp4Any) 
    ss.Printf(" mask: %s", this->netmask.ToString().c_str());
  ss.Printf("]");
  return ss.str();
}


}
