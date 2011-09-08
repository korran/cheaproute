#pragma once

#include "base/common.h"
#include "base/file_descriptor.h"


namespace cheaproute {

class Ip4AddressInfo;

class Netlink {
public:
  Netlink();
  void Init();
  
  void SetDeviceStatus(int device_index, bool up);
  void SetDeviceIp4AddressInfo(int device_index, const Ip4AddressInfo& address_info);
  
private:
  FileDescriptor socket_;
  uint32_t sequence_number_;
};

}