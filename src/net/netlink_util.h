#pragma once

#include "base/common.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include "base/stream.h"

#include <string.h>
#include <assert.h>
#include <linux/rtnetlink.h>
#include <errno.h>

namespace cheaproute {

class Netlink;


typedef unordered_map<int, vector<char> > NetlinkAttributeMap;  

template<typename T>
class NetlinkHeader {
public:
  NetlinkHeader(vector<uint8_t>* buffer, size_t offset) 
    : buffer_(buffer),
      offset_(offset) {
  }
  
  T* get() { return reinterpret_cast<T*>(&(*buffer_)[offset_]); }
  T* operator->() { return get(); }
  
private:
  vector<uint8_t>* buffer_;
  size_t offset_;
};

class NetlinkMessageBuilder {
public:
  NetlinkMessageBuilder(uint16_t type, uint16_t flags) 
      : mainHeader_(&message_, 0) {
    message_.resize(sizeof(nlmsghdr));
    header()->nlmsg_type = type;
    header()->nlmsg_flags = flags;
  }
  nlmsghdr* header() { return mainHeader_.get(); }
  
  template<typename T>
  NetlinkHeader<T> CreateHeader() {
    size_t offset = message_.size();
    message_.resize(message_.size() + sizeof(T));
    return NetlinkHeader<T>(&message_, offset);
  }
  
  const vector<uint8_t>& Build() {
    header()->nlmsg_len = static_cast<uint32_t>(message_.size());
    return message_;
  }
  
  void AddAttribute(uint16_t type, const void* data, size_t len) {
    assert(len < 65536 - sizeof(rtattr));
    rtattr attr;
    attr.rta_type = type;
    attr.rta_len = static_cast<uint16_t>(len + sizeof(attr)) ;
    AppendVectorU8(&message_, &attr, sizeof(attr));
    AppendVectorU8(&message_, data, len);
  }
  
  void SendTo(int fd) {
    
  }
  
private:
  vector<uint8_t> message_;
  NetlinkHeader<nlmsghdr> mainHeader_;
};

class NetlinkReceiver {
public:
  NetlinkReceiver()
      : attributes_valid_(false) {
    buf_.resize(4096);
  }
  
  bool ReceiveFromNonBlock(int fd) {
    return ReceiveFrom(fd, false);
  }
  void ReceiveFrom(int fd) {
    ReceiveFrom(fd, true);
  }

  bool Next() {
    attributes_valid_ = false;
    if (!current_header_) {
      current_header_ = reinterpret_cast<const nlmsghdr*>(&buf_[0]);
      return NLMSG_OK(current_header_, len_);
    }
    current_header_ = NLMSG_NEXT(current_header_, len_);
    return NLMSG_OK(current_header_, len_);
  }
  
  const nlmsghdr* header() const { 
    assert(NLMSG_OK(current_header_, len_));
    return current_header_; 
  }
  
  const struct nlmsgerr* nlmsgerr() const {
    assert(header()->nlmsg_type == NLMSG_ERROR);
    return static_cast<const struct nlmsgerr*>(NLMSG_DATA(header()));
  }
  
  const struct ifinfomsg* ifinfomsg() const {
    assert(header()->nlmsg_type == RTM_NEWLINK || header()->nlmsg_type == RTM_DELLINK);
    return static_cast<const struct ifinfomsg*>(NLMSG_DATA(header()));
  }
  const struct ifaddrmsg* ifaddrmsg() const {
    assert(header()->nlmsg_type == RTM_NEWADDR || header()->nlmsg_type == RTM_DELADDR);
    return static_cast<const struct ifaddrmsg*>(NLMSG_DATA(header()));
  }
  
  const NetlinkAttributeMap& attributes() { 
    if (attributes_valid_)
      return attributes_;
    
    attributes_valid_ = true;
    switch (current_header_->nlmsg_type) {
      case RTM_NEWLINK:
      case RTM_DELLINK:
        ParseAttributes(IFLA_RTA(ifinfomsg()), IFLA_PAYLOAD(current_header_));
        break;
        
      case RTM_NEWADDR:
      case RTM_DELADDR:
        ParseAttributes(IFA_RTA(ifaddrmsg()), IFA_PAYLOAD(current_header_));
        break;
    }
    return attributes_;
  }
  
private:
  bool ReceiveFrom(int fd, bool block) {
    current_header_ = NULL;
    
    struct msghdr msg;
    struct sockaddr_nl addr;
    struct iovec iov;
    
    iov.iov_base = &buf_[0];
    iov.iov_len = buf_.size();

    msg.msg_name = &addr;
    msg.msg_namelen = sizeof(addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    ssize_t bytes_read = recvmsg(fd, &msg, block ? 0 : MSG_DONTWAIT);
    
    if (bytes_read == -1 && errno == EAGAIN && !block)
      return false;
    
    len_ = CheckFdOp(bytes_read, "Receiving netlink message");
    return true;
  }
  
  void ParseAttributes(const rtattr* rta, size_t rtasize) {
    attributes_.clear();
  
    for (; RTA_OK(rta, rtasize); 
        rta = RTA_NEXT(rta, rtasize)) {
      vector<char>& data = attributes_[rta->rta_type];
      data.resize(RTA_PAYLOAD(rta));
      memcpy(&data[0], RTA_DATA(rta), data.size());
    }
    
  }
  vector<uint8_t> buf_;
  const nlmsghdr* current_header_;
  size_t len_;
  NetlinkAttributeMap attributes_;
  bool attributes_valid_;
};

//inline to avoid multiple definition error
inline void SendNetlinkMessage(int fd, const vector<uint8_t>& data) {
  iovec iov;
  iov.iov_base = const_cast<void*>(static_cast<const void*>(&data[0]));
  iov.iov_len = data.size();
  
  sockaddr_nl sa;
  memset (&sa, 0, sizeof(sa));
  sa.nl_family = AF_NETLINK;
  struct msghdr msg = { (void *)&sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
  ssize_t bytes_sent = sendmsg (fd, &msg, 0);
  if (bytes_sent != (ssize_t)iov.iov_len) {
    AbortWithPosixError("sendmsg only sent %zu bytes out of %zu", bytes_sent, iov.iov_len);
  }
}
  
}