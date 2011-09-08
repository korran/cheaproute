
// Copyright 2011 Kor Nielsen

#include "base/common.h"
#include "base/event_loop.h"
#include "base/stream.h"

#include <stdio.h>
#include <unistd.h>
#include "net/netlink.h"
#include "net/netlink_monitor.h"
#include "net/tun_interface.h"
#include "net/interface_activator.h"
#include "base/json_writer.h"
#include "net/json_packet.h"
#include "base/json_reader.h"

namespace cheaproute
{

class TunPlaybackProgram
{
public:
  TunPlaybackProgram(const string& iface_name, const string& packet_log_file)
      : iface_name_(iface_name),
        packet_log_file_(packet_log_file) {
    loop_.reset(new EventLoop());
    netlink_.reset(new Netlink());
    netlink_monitor_.reset(new NetlinkMonitor(loop_.get()));
    tun_.reset(new TunInterface(loop_.get(), iface_name_));
    
    interface_activator_.reset(new InterfaceActivator(netlink_.get(),
                                                      netlink_monitor_.get()));
  }
  
  void Init() {
    // TODO: Remove hard-coded IP address
    interface_activator_->ConfigureInterface(iface_name_.c_str(),  Ip4AddressInfo(
        Ip4Address(192, 168, 6, 1), Ip4Address(192, 168, 6, 255), 24));
    
    netlink_->Init();
    netlink_monitor_->Init();
    
    loop_->Schedule(1.0, bind(&TunPlaybackProgram::Playback, this));
  }
  
  void Run() { 
    loop_->Run(); 
  }
  
private:
  void Playback() {
    JsonReader reader(TransferOwnership(new BufferedInputStream(
        TransferOwnership<InputStream>(
        new FileInputStream(packet_log_file_.c_str())), 4096)));
    
    if (!reader.Next() || reader.token_type() != JSON_StartArray)
      AbortWithMessage("Expected start of array at top of json packet log");
    
    string error;
    vector<uint8_t> packet;
    while (reader.Next() && reader.token_type() != JSON_EndArray) {
      packet.clear();
      if (!DeserializePacket(&reader, &packet, &error)) {
        AbortWithMessage("Error reading packet: %s\n", error.c_str());
      }
      tun_->SendPacket(&packet[0], packet.size());
    }
    loop_->Schedule(1.0, bind(&TunPlaybackProgram::Playback, this));
  }
  TunPlaybackProgram(const TunPlaybackProgram& other);
  scoped_ptr<EventLoop> loop_;
  scoped_ptr<Netlink> netlink_;
  scoped_ptr<NetlinkMonitor> netlink_monitor_;
  scoped_ptr<TunInterface> tun_;
  scoped_ptr<InterfaceActivator> interface_activator_;
  string iface_name_;
  string packet_log_file_;
};

}

int main(int argc, const char *const argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <iface_name> <json_packet_log>\n", argv[0]);
    return -1;
  }
  cheaproute::TunPlaybackProgram program(argv[1], argv[2]);
  program.Init();
  program.Run();
}
