
#include "net/tun_interface.h"
#include "base/event_loop.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

namespace cheaproute {
 
TunInterface::TunInterface(EventLoop* loop, const string& name) {
  CheckNotNull(loop, "loop");
  
  if (name.size() >= IFNAMSIZ) {
    AbortWithMessage("ifname %s is too long; maximum length is %d\n", 
                     name.c_str(), IFNAMSIZ);
  }
  
  fd_.set(CheckFdOp(open("/dev/net/tun", O_RDWR), "Opening TUN device"));
  
  ifreq req;
  memset(&req, 0, sizeof(req));
  req.ifr_flags = IFF_TUN | IFF_NO_PI;
  strcpy(req.ifr_name, name.c_str());
  CheckFdOp(ioctl(fd_.get(), TUNSETIFF, (void*) &req), "setting interface name");
  CheckFdOp(ioctl(fd_.get(), TUNSETNOCSUM, 1), "disabling checksum validation"); 
  
  printf("created TUN interface with name %s\n", req.ifr_name);
  
  ioTask_ = loop->MonitorFd(fd_.get(), kEvRead, bind(&TunInterface::HandleRead, this, _1));
  
  int flags = CheckFdOp(fcntl(fd_.get(), F_GETFL, 0), "Getting socket flags");
  CheckFdOp(fcntl(fd_.get(), F_SETFL, flags | O_NONBLOCK), 
            "Enabling non-blocking behavior on TUN socket");
  
  broadcaster_.reset(new Broadcaster<TunListener>());
}


void TunInterface::SendPacket(const void* data, size_t size) {
  
  ssize_t bytes_written = write(fd_.get(), data, size);
  if (bytes_written == -1) {
    if (errno == EAGAIN) {
      printf("Dropped packet; EAGAIN received from write()\n");
    } else {
      AbortWithPosixError("Unable to write to TUN device");
    }
  }
}

void TunInterface::HandleRead(int flags) { 
  uint8_t buffer[4096];
  ssize_t bytes_read = CheckFdOp(read(fd_.get(), &buffer, sizeof(buffer)), 
                                 "Reading data from TUN device");
  broadcaster_->Broadcast(
    bind(&TunListener::PacketReceived, _1, buffer, bytes_read));
  
}
  
}