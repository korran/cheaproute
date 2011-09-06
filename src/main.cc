
// Copyright 2011 Kor Nielsen

#include "base/common.h"
#include "base/event_loop.h"
#include "base/stream.h"

#include <stdio.h>
#include <unistd.h>
#include "net/netlink.h"
#include "net/tun_interface.h"
#include "base/json_writer.h"
#include "net/json_packet.h"

namespace cheaproute
{

class InterfaceStatusLogger : NetlinkListener {
public:
  InterfaceStatusLogger(Netlink* netlink) {
    listenerHandle_ = netlink->AddListener(this);
  }
  
  void InterfaceChanged(const NetInterfaceInfo& oldInfo, const NetInterfaceInfo& newInfo) {
    printf("Interface changed: %s\n", oldInfo.name.c_str());
  }
  
  void Ip4AddressAdded(const NetInterfaceInfo& info, const Ip4AddressInfo& address) {
    printf("[%d] added IP address %s\n", info.index, address.ToString().c_str());
  }
  void Ip4AddressRemoved(const NetInterfaceInfo& info, const Ip4AddressInfo& address) {
    printf("[%d] removed IP address %s\n", info.index, address.ToString().c_str());
  }
  void Ip6AddressAdded(const NetInterfaceInfo& info, const Ip6Address& address) {
    printf("[%d] added IP address %s\n", info.index, address.ToString().c_str());
  }
  void Ip6AddressRemoved(const NetInterfaceInfo& info, const Ip6Address& address) {
    printf("[%d] removed IP address %s\n", info.index,  address.ToString().c_str());
  }
  void LinkUp(const NetInterfaceInfo& info) {
    printf("[%d] %s link up\n", info.index, info.name.c_str());
  }
  void LinkDown(const NetInterfaceInfo& info) {
    printf("[%d] %s link down\n", info.index, info.name.c_str());
  }
  
private:
  shared_ptr<ListenerHandle> listenerHandle_;
};

class PacketForwarder : public TunListener {
public:
  PacketForwarder(TunInterface* destination) {
    destination_ = CheckNotNull(destination, "destination");
  }
  void PacketReceived(const void* data, size_t size)  {
    destination_->SendPacket(data, size);
  }
  
private:
  TunInterface* destination_;
};

class PacketLogger : public TunListener {
public:
  PacketLogger() {
    FileOutputStream* file_output_stream = new FileOutputStream(STDOUT_FILENO, false);
    
    writer_.reset(new JsonWriter(TransferOwnership(new BufferedOutputStream(
          TransferOwnership<OutputStream>(file_output_stream), 4096)), JsonWriterFlags_Indent));
  }
  
  void PacketReceived(const void* data, size_t size)  {
    SerializePacket(writer_.get(), data, size);
    writer_->Flush();
  }
private:
  scoped_ptr<JsonWriter> writer_;
};

class Program
{
public:
  Program() {
    loop_.reset(new EventLoop());
    netlink_.reset(new Netlink(loop_.get()));
    tun_in_.reset(new TunInterface(loop_.get(), "crIN"));
    tun_out_.reset(new TunInterface(loop_.get(), "crOUT"));
    interface_status_logger_.reset(new InterfaceStatusLogger(netlink_.get()));
    in_out_forwarder_.reset(new PacketForwarder(tun_out_.get()));
    
    packet_logger_.reset(new PacketLogger());
    
    listener_handles_.push_back(tun_in_->AddListener(in_out_forwarder_.get()));
    listener_handles_.push_back(tun_in_->AddListener(packet_logger_.get()));
  }
  
  void Init() {
    netlink_->Init();
  }
  
  void AddInternalInterface(const string& ifname) {
  }
  
  void Run() { 
    loop_->Run(); 
  }
  
private:
  Program(const Program& other);
  scoped_ptr<EventLoop> loop_;
  scoped_ptr<Netlink> netlink_;
  scoped_ptr<TunInterface> tun_in_;
  scoped_ptr<TunInterface> tun_out_;
  scoped_ptr<PacketForwarder> in_out_forwarder_;
  scoped_ptr<PacketLogger> packet_logger_;
  
  scoped_ptr<InterfaceStatusLogger> interface_status_logger_;
  
  vector<shared_ptr<ListenerHandle> > listener_handles_;
};

}

int main(int argc, const char *const argv[]) {
  cheaproute::Program program;
  program.Init();
  program.Run();
}
