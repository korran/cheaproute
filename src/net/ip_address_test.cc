
#include "net/ip_address.h"

#include "gtest/gtest.h"
#include "arpa/inet.h"

namespace cheaproute {

TEST(IpAddressTest, TestIp4AddressSetManually) {
  Ip4Address address;
  const uint8_t addr[] = {169, 254, 32, 156};
  memcpy(address.addr, addr, sizeof(address.addr));
  ASSERT_EQ("169.254.32.156", address.ToString());
}
TEST(IpAddressTest, TestIp4AddressSetAsMemoryInConstructor) {
  const uint8_t addr[] = {169, 254, 32, 156};
  Ip4Address address(addr, sizeof(addr));
  ASSERT_EQ("169.254.32.156", address.ToString());
}
TEST(IpAddressTest, TestIp4AddressSetAsUint32InConstructor) {
  Ip4Address address(htonl(0xA9FE02C7));
  ASSERT_EQ("169.254.2.199", address.ToString());
}

TEST(IpAddressTest, TestIp6AddressSetManually) {
  const uint8_t addr[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  Ip6Address address;
  memcpy(&address.addr, &addr, sizeof(address.addr));
  ASSERT_EQ("::1", address.ToString());
}
TEST(IpAddressTest, TestIp6AddressSetInConstructor) {
  const uint8_t addr[] = {0xFE, 0x80, 0, 0, 0, 0, 0, 0, 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
  Ip6Address address(&addr, sizeof(address.addr));
  ASSERT_EQ("fe80::dead:beef:102:304", address.ToString());
}

}