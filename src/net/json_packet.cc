
#include "base/json_writer.h"
#include "net/ip_address.h"

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

namespace cheaproute {

const uint8_t kProtocolTcp = 6;
const uint8_t kProtocolUdp = 17;
const uint8_t kProtocolIcmp = 1;

const char* kProtocolNames[256] = {
  "HOPOPT", "ICMP", "IGMP", "GGP", "IP", "ST", "TCP", "CBT",
  "EGP", "IGP", "BBN-RCC-MON", "NVP-II", "PUP", "ARGUS", "EMCON", "XNET",
  "CHAOS", "UDP", "MUX", "DCN-MEAS", "HMP", "PRM", "XNS-IDP", "TRUNK-1",
  "TRUNK-2", "LEAF-1", "LEAF-2", "RDP", "IRTP", "ISO-TP4", "NETBLT", "MFE-NSP",
  "MERIT-INP", "DCCP", "3PC", "IDPR", "XTP", "DDP", "IDPR-CMTP", "TP++",
  "IL", "IPv6", "SDRP", "IPv6-Route", "IPv6-Frag", "IDRP", "RSVP", "GRE",
  "MHRP", "BNA", "ESP", "AH", "I-NLSP", "SWIPE", "NARP", "MOBILE",
  "TLSP", "SKIP", "IPv6-ICMP", "IPv6-NoNxt", "IPv6-Opts", NULL, "CFTP", NULL,
  "SAT-EXPAK", "KRYPTOLAN", "RVD", "IPPC", NULL, "SAT-MON", "VISA", "IPCV",
  "CPNX", "CPHB", "WSN", "PVP", "BR-SAT-MON", "SUN-ND", "WB-MON", "WB-EXPAK",
  "ISO-IP", "VMTP", "SECURE-VMTP", "VINES", "TTP", "NSFNET-IGP", "DGP", "TCF",
  "EIGRP", "OSPF", "Sprite-RPC", "LARP", "MTP", "AX.25", "IPIP", "MICP",
  "SCC-SP", "ETHERIP", "ENCAP", NULL, "GMTP", "IFMP", "PNNI", "PIM",
  "ARIS", "SCPS", "QNX", "A/N", "IPComp", "SNP", "Compaq-Peer", "IPX-in-IP",
  "VRRP", "PGM", NULL, "L2TP", "DDX", "IATP", "STP", "SRP", 
  "UTI", "SMP", "SM", "PTP", NULL, "FIRE", "CRTP", "CRUDP",
  "SSCOPMCE", "IPLT", "SPS", "PIPE", "SCTP", "FC", NULL, NULL,
  NULL, NULL, "manet", "HIP", "Shim6", NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

const char* kIcmpTypeNames[32] = {
  "echoReply", NULL, "destinationUnreachable", "sourceQuench", 
  "redirectMessage", NULL, NULL, NULL,
  "echoRequest", "routerAdvertisement", "routerSolicitation", "timeExceeded",
  "badIpHeader", "timestamp", "timestampReply", "infoRequest",
  "infoReply", "addressMaskRequest", "addressMaskReply", NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, "traceroute", NULL
};

const char* kIcmpDestinationUnreachableCodeNames[] = {
  "destinationNetworkUnreachable", "destinationHostUnreachable",
  "destinationProtocolUnreachable", "destinationPortUnreachable",
  "fragmentationRequired", "sourceRouteFailed",
  "destinationNetworkUnknown", "destinationHostUnknown",
  "sourceHostIsolated", "networkAdministrativelyProhibited",
  "hostAdministrativelyProhibited", "networkUnreachableForTos",
  "communicationAdministrativelyProhibited"
};

const char* kIcmpRedirectMessageCodeNames[] = {
  "redirectDatagramForHost", "redirectDatagramForNetwork",
  "redirectDatagramForTosAndNetwork", "redirectDatagramForTosAndHost"
};

const char* kIcmpBadIpHeaderCodeNames[] = {
  "pointerIndicatesTheError", "missingARequiredOption",
  "badLength"
};

static void WriteIpFlags(JsonWriter* writer, int flags) {
  writer->BeginArray();
  if (flags & IP_DF)
    writer->WriteString("DF");
  if (flags & IP_MF)
    writer->WriteString("MF");
  writer->EndArray();
}
static void WriteTcpFlags(JsonWriter* writer, const tcphdr* hdr) {
  writer->BeginArray();
  if (hdr->urg)
    writer->WriteString("URG");
  if (hdr->ack)
    writer->WriteString("ACK");
  if (hdr->psh)
    writer->WriteString("PSH");
  if (hdr->rst)
    writer->WriteString("RST");
  if (hdr->syn)
    writer->WriteString("SYN");
  if (hdr->fin)
    writer->WriteString("FIN");
  writer->EndArray();
}

template<typename T>
void WriteFriendlyStringOrInt(JsonWriter* writer, const T& lookup_array, int value) {
  const char* str_value = NULL;
  
  if (value >= 0 && value < static_cast<int>(ArrayLength(lookup_array))) {
    str_value = lookup_array[value];
  }
  if (str_value) {
    writer->WriteString(str_value);
  } else {
    writer->WriteInteger(value);
  }
}


static bool IsPlainText(const void* data, size_t size) {
  const uint8_t* p = static_cast<const uint8_t*>(data);
  const uint8_t* end = p + size;
  for (; p < end; p++) {
    if (*p < ' ' && *p != '\r' && *p != '\n' && *p != '\t') {
      return false;
    }
  }
  return true;
}

static void SerializeBytesAsText(JsonWriter* writer, const void* data, size_t size) {
  writer->BeginObject();
  writer->WritePropertyName("type");
  writer->WriteString("text");
  writer->WritePropertyName("data");
  writer->BeginArray();
  const uint8_t* p = static_cast<const uint8_t*>(data);
  const uint8_t* end = p + size;
  string line;
  for (; p < end; p++) {
    line.push_back(*p);
    if (*p == '\n') {
      writer->WriteString(line);
      line.clear();
    }
  }
  if (line.size() > 0) {
    writer->WriteString(line);
  }
  writer->EndArray();
  writer->EndObject();
}

static void SerializeBytesAsHex(JsonWriter* writer, const void* data, size_t size) {
  const ssize_t bytes_per_line = 16;
  writer->BeginObject();
  writer->WritePropertyName("type");
  writer->WriteString("hex");
  writer->WritePropertyName("data");
  
  const uint8_t* p = static_cast<const uint8_t*>(data);
  const uint8_t* end = p + size;
  writer->BeginArray();
  while (p < end) {
    string line = FormatHex(p, std::min(end - p, bytes_per_line));
    writer->WriteString(line);
    p += 16;
  }
  writer->EndArray();
  writer->EndObject();
}

static void SerializeBytes(JsonWriter* writer, const void* data, size_t size) {
  if (IsPlainText(data, size)) {
    SerializeBytesAsText(writer, data, size);
  } else {
    SerializeBytesAsHex(writer, data, size);
  }
}

static void WriteIcmpCode(JsonWriter* writer, int type, int code) {
  switch (type) {
    case ICMP_DEST_UNREACH:
      WriteFriendlyStringOrInt(writer, kIcmpDestinationUnreachableCodeNames, code);
      break;
      
    case ICMP_REDIRECT:
      WriteFriendlyStringOrInt(writer, kIcmpRedirectMessageCodeNames, code);
      break;
     
    case ICMP_PARAMETERPROB:
      WriteFriendlyStringOrInt(writer, kIcmpBadIpHeaderCodeNames, code);
      break;
      
    default:
      writer->WriteInteger(code);
      break;
  }
}

static size_t WriteIpHeader(JsonWriter* writer, const iphdr* ip_header) {
  writer->BeginObject();
  writer->WritePropertyName("version");
  writer->BeginPack();
  writer->WriteInteger(static_cast<int>(ip_header->version));
  writer->WritePropertyName("tos");
  writer->WriteInteger(ip_header->tos);
  writer->WritePropertyName("id");
  writer->WriteInteger(htons(ip_header->id));
  writer->WritePropertyName("flags");
  WriteIpFlags(writer, htons(ip_header->frag_off));
  writer->EndPack();
  
  writer->WritePropertyName("fragmentOffset");
  writer->BeginPack();
  writer->WriteInteger(htons(ip_header->frag_off) & IP_OFFMASK);
  writer->WritePropertyName("ttl");
  writer->WriteInteger(ip_header->ttl);
  writer->WritePropertyName("protocol");
  WriteFriendlyStringOrInt<>(writer, kProtocolNames, ip_header->protocol);
  writer->EndPack();
  
  writer->WritePropertyName("source");
  writer->BeginPack();
  writer->WriteString(Ip4Address(ip_header->saddr).ToString());
  writer->WritePropertyName("destination");
  writer->WriteString(Ip4Address(ip_header->daddr).ToString());
  writer->EndPack();
  
  // TODO: Write options
  
  writer->EndObject();
  
  return ip_header->ihl * 4;
}

static uint32_t DerefU32BE(const void* void_p) {
  const uint8_t* p = static_cast<const uint8_t*>(void_p);
  return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3] << 0;
}
static uint32_t DerefU16BE(const void* void_p) {
  const uint8_t* p = static_cast<const uint8_t*>(void_p);
  return p[0] << 8 | p[1] << 0;
}

static void WriteTcpOption(JsonWriter* writer, const char* name, uint32_t value) {
  writer->BeginArray();
  writer->WriteString(name);
  writer->WriteInteger(static_cast<int64_t>(value));
  writer->EndArray();
}

static size_t WriteTcpHeader(JsonWriter* writer, const tcphdr* tcp_header, size_t size_limit) {
  assert(size_limit >= sizeof(tcp_header));
  
  writer->BeginObject();
  writer->WritePropertyName("sourcePort");
  writer->BeginPack();
  writer->WriteInteger(htons(tcp_header->source));
  writer->WritePropertyName("destPort");
  writer->WriteInteger(htons(tcp_header->dest));
  writer->EndPack();
  
  writer->WritePropertyName("seqNumber");
  writer->BeginPack();
  writer->WriteInteger(static_cast<int64_t>(htonl(tcp_header->seq)));
  if (tcp_header->ack) {
    writer->WritePropertyName("ackNumber");
    writer->WriteInteger(static_cast<int64_t>(htonl(tcp_header->ack_seq)));
  }
  writer->EndPack();
  
  
  writer->WritePropertyName("flags");
  writer->BeginPack();
  WriteTcpFlags(writer, tcp_header);
  writer->WritePropertyName("windowSize");
  writer->BeginPack();
  writer->WriteInteger(htons(tcp_header->window));
  if (tcp_header->urg) {
    writer->WritePropertyName("urgentPointer");
    writer->WriteInteger(htons(tcp_header->urg_ptr));
  }
  writer->EndPack();
  
  size_t total_header_size = tcp_header->doff * 4;
  const uint8_t* options = 
    reinterpret_cast<const uint8_t*>(tcp_header) + sizeof(tcphdr);
  size_t options_size = std::min(size_limit - sizeof(tcphdr), total_header_size);
  
  if (options_size > 0) {
    writer->WritePropertyName("options");
    writer->BeginPack();
    writer->BeginArray();
    
    const uint8_t* options_end = options + options_size;

    for (const uint8_t* p = options; p < options_end; ) {
      uint8_t option_type = *p;
      
      if (option_type == TCPOPT_NOP) {
        writer->WriteString("NOP");
        p++;
        continue;
      }
      if (option_type == TCPOPT_EOL) {
        writer->WriteString("EOL");
        break;
      }
      if (p+1 >= options_end)
        break;
      
      uint8_t option_size = p[1];
      if (p + option_size >= options_end)
        break;
      
      switch (option_type) {
        case TCPOPT_MAXSEG:
          if (option_size != TCPOLEN_MAXSEG)
            break;
          WriteTcpOption(writer, "maxSegmentSize", DerefU16BE(&p[2]));
          break;
          
        case TCPOPT_WINDOW:
          if (option_size != TCPOLEN_WINDOW)
            break;
          WriteTcpOption(writer, "windowScale", p[2]);
          break;
          
        case TCPOPT_SACK_PERMITTED:
          if (option_size != TCPOLEN_SACK_PERMITTED)
            break;
          writer->WriteString("sackPermitted");
          break;
          
        case TCPOPT_SACK:
          writer->BeginArray();
          writer->WriteString("selectiveAck");
          SerializeBytesAsHex(writer, &p[2], option_size - 2);
          writer->EndArray();
          break;
          
        case TCPOPT_TIMESTAMP:
          if (option_size != TCPOLEN_TIMESTAMP)
            break;
          writer->BeginArray();
          writer->WriteString("timestamp");
          writer->WriteInteger(DerefU32BE(p + 2));
          writer->WriteInteger(DerefU32BE(p + 6));
          writer->EndArray();
          break;
      }
      
      p += option_size;
    }
    writer->EndArray();
    writer->EndPack();
  }
  writer->EndObject();
  return total_header_size;
}

static size_t WriteUdpHeader(JsonWriter* writer, const udphdr* udp_header) {
  writer->BeginObject();
  writer->WritePropertyName("sourcePort");
  writer->WriteInteger(htons(udp_header->source));
  writer->WritePropertyName("destPort");
  writer->WriteInteger(htons(udp_header->dest));
  writer->EndObject();
  return sizeof(udp_header);
}

static size_t WriteIcmpHeader(JsonWriter* writer, const icmphdr* icmp_header) {
  writer->BeginObject();
  writer->WritePropertyName("type");
  writer->BeginPack();
  WriteFriendlyStringOrInt(writer, kIcmpTypeNames, icmp_header->type);
  writer->WritePropertyName("code");
  WriteIcmpCode(writer, icmp_header->type, icmp_header->code);
  writer->EndPack();
  
  switch (icmp_header->type) {
    case ICMP_ECHO: 
    case ICMP_ECHOREPLY:
    case ICMP_TIMESTAMP:
    case ICMP_TIMESTAMPREPLY:
    case ICMP_ADDRESS:
    case ICMP_ADDRESSREPLY:
      writer->WritePropertyName("identifier");
      writer->BeginPack();
      writer->WriteInteger(htons(icmp_header->un.echo.id));
      writer->WritePropertyName("sequenceNumber");
      writer->WriteInteger(htons(icmp_header->un.echo.sequence));
      writer->EndPack();
      break;
      
    case ICMP_DEST_UNREACH:
      writer->WritePropertyName("nextHopMtu");
      writer->WriteInteger(htons(icmp_header->un.frag.mtu));
      break;
      
    case ICMP_REDIRECT:
      writer->WritePropertyName("gateway");
      writer->WriteString(Ip4Address(icmp_header->un.gateway).ToString());
      break;
  }
  writer->EndObject();
  return sizeof(icmphdr);
}


void SerializePacket(JsonWriter* writer, const void* packet, size_t size) {
  writer->BeginObject();

  if (size >= sizeof(iphdr)) {
    const iphdr* header = static_cast<const iphdr*>(packet);

    if (header->ihl >= 5) {
      writer->WritePropertyName("ip");

      size_t next_header_offset = WriteIpHeader(writer, header);
      size_t header2_size = 0;
      if (header->protocol == kProtocolTcp && 
          size >= next_header_offset + sizeof(tcphdr)) {
        
        const tcphdr* tcp_header = reinterpret_cast<const tcphdr*>(
            static_cast<const char*>(packet) + next_header_offset);
        
        writer->WritePropertyName("tcp");
        header2_size = WriteTcpHeader(writer, tcp_header, 
                                                size - next_header_offset);
      }
      else if (header->protocol == kProtocolUdp && 
               size >= next_header_offset + sizeof(udphdr)) {
        
        const udphdr* udp_header = reinterpret_cast<const udphdr*>(
            static_cast<const char*>(packet) + next_header_offset);
        
        writer->WritePropertyName("udp");
        header2_size = WriteUdpHeader(writer, udp_header);
      } 
      else if (header->protocol == kProtocolIcmp && 
               size >= next_header_offset + sizeof(icmphdr)) {
        
        const icmphdr* icmp_header = reinterpret_cast<const icmphdr*>(
            static_cast<const char*>(packet) + next_header_offset);
        
        writer->WritePropertyName("icmp");
        header2_size = WriteIcmpHeader(writer, icmp_header);
      }
    
      size_t data_offset = next_header_offset + header2_size;
      
      if (size > data_offset) {
        writer->WritePropertyName("data");
        SerializeBytes(writer, static_cast<const char*>(packet) + data_offset, 
                        size - data_offset);
      }
    }
  }
  writer->EndObject();
}

}