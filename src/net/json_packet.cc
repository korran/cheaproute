
#include "base/json_writer.h"
#include "net/ip_address.h"

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <base/json_reader.h>
#include <arpa/inet.h>

#include <stdarg.h>
#include <inttypes.h>
#include <limits>

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

const int kTcpFlagFin = (1 << 0);
const int kTcpFlagSyn = (1 << 1);
const int kTcpFlagRst = (1 << 2);
const int kTcpFlagPsh = (1 << 3);
const int kTcpFlagAck = (1 << 4);
const int kTcpFlagUrg = (1 << 5);
const int kTcpFlagEce = (1 << 6);
const int kTcpFlagCwr = (1 << 7);
const int kTcpFlagNs = (1 << 8);

const char* kTcpFlags[] = {
  "FIN", "SYN", "RST", "PSH", "ACK", "URG", "ECE", "CWR", "NS"
};

const char* kTcpOptions[] = {
  "EOL", "NOP", "maxSegmentSize", "windowScale", "sackPermitted", 
  "selectedAck", NULL, NULL, "timestamp"
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


static const unordered_map<string, int>* CreateIpFlagLookupTable() {
  unordered_map<string, int>* result = new unordered_map<string, int>();
  (*result)["DF"] = IP_DF;
  (*result)["MF"] = IP_MF;
  return result;
}
static const unordered_map<string, int>* GetIpFlagLookupTable() {
  const static unordered_map<string, int>* result = CreateIpFlagLookupTable();
  return result;
}

template<typename T>
static const unordered_map<string, T>* CreateLookupTable(const char** array, size_t array_size) {
  unordered_map<string, T>* result = new unordered_map<string, T>();
  
  for (size_t i = 0; i < array_size; i++) {
    const char* value = array[i];
    if (value) {
      (*result)[string(value)] = static_cast<T>(i);
    }
  }
  return result;
}
template<typename T>
static const unordered_map<string, T>* CreateFlagLookupTable(const char** array, size_t array_size) {
  unordered_map<string, T>* result = new unordered_map<string, T>();
  
  for (size_t i = 0; i < array_size; i++) {
    const char* value = array[i];
    if (value) {
      (*result)[string(value)] = 1 << static_cast<T>(i);
    }
  }
  return result;
}


static const unordered_map<string, int>* GetTcpFlagLookupTable() {
  const static unordered_map<string, int>* result = 
    CreateFlagLookupTable<int>(kTcpFlags, ArrayLength(kTcpFlags));
  return result;
}
static const unordered_map<string, uint8_t>* GetTcpOptionTable() {
  const static unordered_map<string, uint8_t>* result = 
      CreateLookupTable<uint8_t>(kTcpOptions, ArrayLength(kTcpOptions));
  return result;
}
static const unordered_map<string, uint8_t>* GetIcmpTypeLookupTable() {
  const static unordered_map<string, uint8_t>* result = 
      CreateLookupTable<uint8_t>(kIcmpTypeNames, ArrayLength(kIcmpTypeNames));
  return result;
}
static const unordered_map<string, uint8_t>* GetIcmpDestinationUnreacheableCodeNameLookupTable() {
  const static unordered_map<string, uint8_t>* result = 
      CreateLookupTable<uint8_t>(kIcmpDestinationUnreachableCodeNames, 
                             ArrayLength(kIcmpDestinationUnreachableCodeNames));
  return result;
}
static const unordered_map<string, uint8_t>* GetIcmpRedirectMessageCodeNamesLookupTable() {
  const static unordered_map<string, uint8_t>* result = 
      CreateLookupTable<uint8_t>(kIcmpRedirectMessageCodeNames, 
                             ArrayLength(kIcmpRedirectMessageCodeNames));
  return result;
}
static const unordered_map<string, uint8_t>* GetIcmpBadIpHeaderCodeNames() {
  const static unordered_map<string, uint8_t>* result = 
      CreateLookupTable<uint8_t>(kIcmpBadIpHeaderCodeNames, 
                             ArrayLength(kIcmpBadIpHeaderCodeNames));
  return result;
}
static const unordered_map<string, uint8_t>* GetProtocolNameLookupTable() {
  const static unordered_map<string, uint8_t>* result = 
      CreateLookupTable<uint8_t>(kProtocolNames, ArrayLength(kProtocolNames));
  return result;
}

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
  writer->WriteInteger(htonl(tcp_header->seq));
  if (tcp_header->ack) {
    writer->WritePropertyName("ackNumber");
    writer->WriteInteger(htonl(tcp_header->ack_seq));
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
  size_t options_size = std::min(size_limit - sizeof(tcphdr), 
                                 std::max(static_cast<size_t>(20), total_header_size) - 20);
  
  if (options_size > 0) {
    writer->WritePropertyName("options");
    writer->BeginPack();
    writer->BeginArray();
    
    const uint8_t* options_end = options + options_size;

    for (const uint8_t* p = options; p < options_end; ) {
      uint8_t option_type = *p;
      
      if (option_type == TCPOPT_NOP) {
        writer->BeginArray();
        writer->WriteString("NOP");
        writer->EndArray();
        p++;
        continue;
      }
      if (option_type == TCPOPT_EOL) {
        writer->BeginArray();
        writer->WriteString("EOL");
        writer->EndArray();
        break;
      }
      if (p+1 >= options_end)
        break;
      
      uint8_t option_size = p[1];
      if (p + option_size > options_end)
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
          writer->BeginArray();
          writer->WriteString("sackPermitted");
          writer->EndArray();
          break;
          
        // TODO: Support TCPOPT_SACK
          
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

static bool Error(string* out_error_string, const char* format_string, ...)
__attribute__ ((format (printf, 2, 3)));

static bool Error(string* out_error_string, const char* format_string, ...) {
  if (out_error_string) {
    va_list args;
    va_start(args, format_string);
    *out_error_string = VStrPrintf(format_string, args);
    va_end(args);
  }
  return false;
}

static bool PrefixError(string* out_error_string, const char* format_string, ...) {
  if (out_error_string) {
    va_list args;
    va_start(args, format_string);
    *out_error_string = VStrPrintf(format_string, args) + ": " + *out_error_string;
    va_end(args);
  }
  return false;
}

static bool ExpectNextJsonToken(JsonReader* reader, string* out_err) {
  if (!reader->Next()) 
    return Error(out_err, "Unexpected end of file");
  return true;
}

static bool ExpectCurrentJsonToken(JsonReader* reader, JsonToken expected_token, 
                                string* out_err) {
  if (reader->token_type() != expected_token) {
    return Error(out_err, "Unexpected token '%s', expected token '%s'",
                 GetJsonTokenName(reader->token_type()),
                 GetJsonTokenName(expected_token));
  }
  return true;
}

static bool ExpectNextJsonToken(JsonReader* reader, JsonToken expected_token, 
                                string* out_err) {
  if (!ExpectNextJsonToken(reader, out_err))
    return false;
  
  return ExpectCurrentJsonToken(reader, expected_token, out_err);
}

static bool ExpectCurrentPropertyName(JsonReader* reader, const char* property_name,
                               string* out_err) {
  if (reader->token_type() != JSON_PropertyName)
    return Error(out_err, "expected property named %s, was token %s",
                 property_name, GetJsonTokenName(reader->token_type()));
  
  if (reader->str_value() != string(property_name)) 
    return Error(out_err, "expected property named '%s', instead it "
                 "was named '%s'", property_name, reader->str_value().c_str());
  
  return true;
}

static bool ExpectNextPropertyName(JsonReader* reader, const char* property_name,
                               string* out_err) {
  if (!ExpectNextJsonToken(reader, out_err))
    return false;
  
  return ExpectCurrentPropertyName(reader, property_name, out_err);
}

template<typename T>
bool ExpectNextEnumProperty(JsonReader* reader, const char* property_name,
                               const unordered_map<string, T>* lookup_table,
                               T* out_result, string* out_err) {
  if (!ExpectNextPropertyName(reader, property_name, out_err))
    return false;
  
  if (!ExpectNextEnumValue(reader, lookup_table, out_result, out_err)) {
    return PrefixError(out_err, "Error with property '%s'", property_name);
  }
  
  return true;
}

template<typename T>
bool ExpectNextEnumValue(JsonReader* reader, 
                               const unordered_map<string, T>* lookup_table,
                               T* out_result, string* out_err) {
  if (!ExpectNextJsonToken(reader, out_err))
    return false;
  
  typename unordered_map<string, T>::const_iterator i = 
      lookup_table->find(reader->str_value());
  
  if (i == lookup_table->end()) {
    return Error(out_err, "unknown value '%s'", 
                 reader->str_value().c_str());
  }
  *out_result = i->second;
  return true;
}

static bool ExpectCurrentInt64Value(JsonReader* reader, int64_t min_value, 
                                       int64_t max_value, int64_t* out_result,
                                       string* out_err) {
  
  if (!ExpectCurrentJsonToken(reader, JSON_Integer, out_err)) {
    if (reader->error_code() == JsonError_OutOfRange) {
      return Error(out_err, "integer out of range");
    }
    return false;
  }
  int64_t value = reader->int_value();
  
  if (value < min_value || value > max_value) {
    return Error(out_err,  "expected integer "
                 "between %"PRId64" and %"PRId64"; was %"PRId64, 
                 min_value, max_value, value);
  }
  *out_result = value;
  return true;
}

template<typename T>
T ToBigEndian(T value) {
  return value;
}
template<>
uint16_t ToBigEndian(uint16_t value) {
  return htons(value);
}
template<>
uint32_t ToBigEndian(uint32_t value) {
  return htonl(value);
}

template<typename T>
bool ExpectCurrentValue(JsonReader* reader, T min_value, T max_value, 
                        T* out_result, string* out_err) {
  int64_t result;
  if (!ExpectCurrentInt64Value(reader, min_value, max_value, &result, out_err))
    return false;
  *out_result = ToBigEndian(static_cast<T>(result));
  return true;
}

template<typename T>
bool ExpectCurrentValue(JsonReader* reader, T* out_result, string* out_err) {
  return ExpectCurrentValue(reader, std::numeric_limits<T>::min(),
                        std::numeric_limits<T>::max(), out_result, out_err);
}

template<typename T>
bool ExpectNextValue(JsonReader* reader, T min_value, T max_value, 
                     T* out_result, string* out_err) {
  if (!ExpectNextJsonToken(reader, out_err))
    return false;
  return ExpectCurrentValue(reader, min_value, max_value, out_result, out_err);
}

template<typename T>
bool ExpectNextValue(JsonReader* reader, T* out_result, string* out_err) {
  if (!ExpectNextJsonToken(reader, out_err))
    return false;
  return ExpectCurrentValue(reader, out_result, out_err);
}

template<typename T>
bool ExpectCurrentProperty(JsonReader* reader, const char* property_name, 
                               T min_value, T max_value, T* out_result, 
                               string* out_err) {
  if (!ExpectCurrentPropertyName(reader, property_name, out_err))
    return false;
  
  if (!ExpectNextValue(reader, min_value, max_value, out_result, out_err)) 
    return PrefixError(out_err, "Error parsing property %s", property_name);
  
  return true;
}

template<typename T>
bool ExpectNextProperty(JsonReader* reader, const char* property_name, 
                               T min_value, T max_value, T* out_result, 
                               string* out_err) {
  if (!ExpectNextJsonToken(reader, out_err))
    return false;
  
  return ExpectCurrentProperty(reader, property_name, min_value, max_value, 
                               out_result, out_err);
}

template<typename T>
bool ExpectCurrentProperty(JsonReader* reader, const char* property_name, 
                               T* out_result, string* out_err) {
  if (!ExpectCurrentPropertyName(reader, property_name, out_err))
    return false;
  
  if (!ExpectNextValue(reader, out_result, out_err))
    return PrefixError(out_err, "Error with property '%s'", property_name);
  return true;
}

template<typename T>
bool ExpectNextProperty(JsonReader* reader, const char* property_name, 
                               T* out_result, string* out_err) {
  if (!ExpectNextJsonToken(reader, out_err))
    return false;
  return ExpectCurrentProperty(reader, property_name, out_result, out_err);
}


static bool ExpectIp4Value(JsonReader* reader, uint32_t* out_result, string* out_err) {
  if (!ExpectNextJsonToken(reader, JSON_String, out_err))
    return false;
  
  struct in_addr sock_addr;
  const string& str_value = reader->str_value();
  if (inet_pton(AF_INET, str_value.c_str(), &sock_addr) != 1) {
    return Error(out_err, "Unable to parse IPv4 address '%s'", str_value.c_str());
  }
  *out_result = sock_addr.s_addr;
  return true;
}

bool DeserializeFlags(JsonReader* reader, const unordered_map<string, int>* lookup_table,
                      int* out_result, string* out_err) {
  if (!ExpectNextJsonToken(reader, JSON_StartArray, out_err))
    return false;
  int result = 0;
  while (true) {
    if (!ExpectNextJsonToken(reader, out_err))
      return false;
    if (reader->token_type() == JSON_EndArray) 
      break;
    
    if (reader ->token_type() != JSON_String)
      return Error(out_err, "Unexpected token while deserializing flags");
    
    unordered_map<string, int>::const_iterator i = 
        lookup_table->find(reader->str_value());
        
    if (i == lookup_table->end()) 
      return Error(out_err, "Unknown flag %s", reader->str_value().c_str());
    
    result |= i->second;
  }
  *out_result = result;
  return true;
}

static bool DeserializeIp4Header(JsonReader* reader, vector<uint8_t>* dest_buffer, string* out_err) {
  if (!ExpectNextJsonToken(reader, JSON_StartObject, out_err))
    return false;
    
  iphdr ip_header;
  memset(&ip_header, 0, sizeof(ip_header));
  
  uint8_t version;
  if (!ExpectNextProperty<uint8_t>(reader, "version", 4, 4, &version, out_err))
    return false;
    
  ip_header.version = version & 0x0f;
  ip_header.ihl = 5;
  if (!ExpectNextProperty<uint8_t>(reader, "tos", 0, 255, &ip_header.tos, out_err))
    return false;
  if (!ExpectNextProperty<uint16_t>(reader, "id", &ip_header.id, out_err))
    return false;
  
  if (!ExpectNextPropertyName(reader, "flags", out_err))
    return false;
  
  int flags;
  if (!DeserializeFlags(reader, GetIpFlagLookupTable(), &flags, out_err))
    return PrefixError(out_err, "While reading property 'flags' in ip header");
  
  if (!ExpectNextProperty<uint16_t>(reader, "fragmentOffset", 0, 8191, 
                                &ip_header.frag_off, out_err)) {
    return false;
  }
  
  ip_header.frag_off |= htons(static_cast<uint16_t>(flags));
  
  if (!ExpectNextProperty<uint8_t>(reader, "ttl", &ip_header.ttl, out_err))
    return false;
  
  if (!ExpectNextEnumProperty(reader, "protocol", GetProtocolNameLookupTable(), 
                          &ip_header.protocol,  out_err)) {
    return false;
  }
  
  if (!ExpectNextPropertyName(reader, "source", out_err))
    return false;
  if (!ExpectIp4Value(reader, &ip_header.saddr, out_err))
    return PrefixError(out_err, "In property 'source'");
  
  if (!ExpectNextPropertyName(reader, "destination", out_err))
    return false;
  if (!ExpectIp4Value(reader, &ip_header.daddr, out_err))
    return PrefixError(out_err, "In property 'destination'");
  
  // TODO: Deserialize IP options
  
  if (!ExpectNextJsonToken(reader, JSON_EndObject, out_err))
    return PrefixError(out_err, "While reading IP header");
  
  AppendVectorU8(dest_buffer, &ip_header, sizeof(ip_header));
  return true;
}

static bool DeserializeTcpHeaderOptions(JsonReader* reader, vector<uint8_t>* dest_buffer, string* out_err) {
  if (reader->token_type() == JSON_EndObject)
    return true;

  if (!ExpectCurrentPropertyName(reader, "options", out_err))
    return false;
  if (!ExpectNextJsonToken(reader, JSON_StartArray, out_err))
    return false;
  if (!ExpectNextJsonToken(reader, out_err))
    return false;
  
  while (reader->token_type() != JSON_EndArray) {
    if (!ExpectCurrentJsonToken(reader, JSON_StartArray, out_err))
      return false;
    
    uint8_t option_type;
    if (!ExpectNextEnumValue(reader, GetTcpOptionTable(), &option_type, out_err))
      return false;
    
    switch (option_type) {
      case TCPOPT_EOL:
      case TCPOPT_NOP:
        dest_buffer->push_back(option_type);
        break;
        
      case TCPOPT_SACK_PERMITTED:
        dest_buffer->push_back(option_type);
        dest_buffer->push_back(TCPOLEN_SACK_PERMITTED);
        break;
        
      case TCPOPT_WINDOW:
      case TCPOPT_MAXSEG: {
        dest_buffer->push_back(option_type);
        dest_buffer->push_back(4);
        uint16_t value;
        if (!ExpectNextValue<uint16_t>(reader, &value, out_err))
          return false;
        AppendVectorU8(dest_buffer, &value, sizeof(value));
        break;
      }
      
      case TCPOPT_SACK: {
        return Error(out_err, "Selective acknowledgements are not supported");
      }
      
      case TCPOPT_TIMESTAMP: {
        dest_buffer->push_back(option_type);
        dest_buffer->push_back(TCPOLEN_TIMESTAMP);
        uint32_t value1;
        uint32_t value2;
        if (!ExpectNextValue<uint32_t>(reader, &value1, out_err))
          return false;
        if (!ExpectNextValue<uint32_t>(reader, &value2, out_err))
          return false;
        AppendVectorU8(dest_buffer, &value1, sizeof(value1));
        AppendVectorU8(dest_buffer, &value2, sizeof(value2));
        break;
      }
    }
    if (!ExpectNextJsonToken(reader, JSON_EndArray, out_err))
      return false;
    if (!ExpectNextJsonToken(reader, out_err))
      return false;
  }
  if (!ExpectNextJsonToken(reader, JSON_EndObject, out_err))
    return false;
  
  return true;
}

static bool DeserializeTcpHeader(JsonReader* reader, vector<uint8_t>* dest_buffer, string* out_err) {
  if (!ExpectNextJsonToken(reader, JSON_StartObject, out_err))
    return false;
  
  tcphdr tcp_header;
  memset(&tcp_header, 0, sizeof(tcp_header));
  
  if (!ExpectNextProperty<uint16_t>(reader, "sourcePort", &tcp_header.source, out_err))
    return false;
  if (!ExpectNextProperty<uint16_t>(reader, "destPort", &tcp_header.dest, out_err))
    return false;
  if (!ExpectNextProperty<uint32_t>(reader, "seqNumber", &tcp_header.seq, out_err))
    return false;
  
  if (!ExpectNextJsonToken(reader, JSON_PropertyName, out_err))
    return false;
  
  if (reader->str_value() == "ackNumber") {
    if (!ExpectCurrentProperty<uint32_t>(reader, "ackNumber", &tcp_header.ack_seq, out_err))
      return false;
    if (!ExpectNextJsonToken(reader, JSON_PropertyName, out_err))
    return false;
  }
  
  if (!ExpectCurrentPropertyName(reader, "flags", out_err))
    return false;
  
  int tcp_flags;
  if (!DeserializeFlags(reader, GetTcpFlagLookupTable(), &tcp_flags, out_err))
    return false;
  tcp_header.fin = static_cast<bool>(tcp_flags & kTcpFlagFin);
  tcp_header.syn = static_cast<bool>(tcp_flags & kTcpFlagSyn);
  tcp_header.rst = static_cast<bool>(tcp_flags & kTcpFlagRst);
  tcp_header.psh = static_cast<bool>(tcp_flags & kTcpFlagPsh);
  tcp_header.ack = static_cast<bool>(tcp_flags & kTcpFlagAck);
  tcp_header.urg = static_cast<bool>(tcp_flags & kTcpFlagUrg);
  
  if (!ExpectNextProperty<uint16_t>(reader, "windowSize", &tcp_header.window, out_err))
    return false;
  
  if (tcp_header.urg) {
    if (!ExpectNextProperty<uint16_t>(reader, "urgentPointer", &tcp_header.urg_ptr, out_err))
      return false;
  }
  
  if (!ExpectNextJsonToken(reader, out_err))
    return false;
  
  size_t tcp_header_offset = dest_buffer->size();
  AppendVectorU8(dest_buffer, &tcp_header, sizeof(tcp_header));
  
  size_t options_offset = dest_buffer->size();
  if (!DeserializeTcpHeaderOptions(reader, dest_buffer, out_err))
    return false;
  size_t options_size = dest_buffer->size() - options_offset;
  
  // TODO: Perhaps we should automatically add NOPs if the JSON doesn't 
  //       align things correctly
  if (options_size  % 4 != 0)
    return Error(out_err, "TCP header options are not padded correctly");
  
  tcphdr* p_tcp_header = reinterpret_cast<tcphdr*>(&(*dest_buffer)[tcp_header_offset]);
  p_tcp_header->doff = (5 + options_size / 4) & 0x0f;
  
  return true;
}

static bool DeserializeUdpHeader(JsonReader* reader, vector<uint8_t>* dest_buffer, string* out_err) {
  if (!ExpectNextJsonToken(reader, JSON_StartObject, out_err))
    return false;
  
  udphdr udp_header;
  memset(&udp_header, 0, sizeof(udp_header));
  
  if (!ExpectNextProperty<uint16_t>(reader, "sourcePort", &udp_header.source, out_err))
    return false;
  if (!ExpectNextProperty<uint16_t>(reader, "destPort", &udp_header.dest, out_err))
    return false;
  
  if (!ExpectNextJsonToken(reader, JSON_EndObject, out_err))
    return false;
  
  AppendVectorU8(dest_buffer, &udp_header, sizeof(udp_header));
  return true;
}

static bool DeserializeIcmpHeader(JsonReader* reader, vector<uint8_t>* dest_buffer, string* out_err) {
  if (!ExpectNextJsonToken(reader, JSON_StartObject, out_err))
    return false;
  
  icmphdr icmp_header;
  memset(&icmp_header, 0, sizeof(icmp_header));
  if (!ExpectNextEnumProperty(reader, "type", GetIcmpTypeLookupTable(), 
                          &icmp_header.type, out_err)) {
    return false;
  }
  
  switch (icmp_header.type) {
    case ICMP_DEST_UNREACH:
      if (!ExpectNextEnumProperty(reader, "code", GetIcmpDestinationUnreacheableCodeNameLookupTable(),
                         &icmp_header.code, out_err)) {
        return false;
      }
      break;
      
    case ICMP_REDIRECT:
      if (!ExpectNextEnumProperty(reader, "code", GetIcmpRedirectMessageCodeNamesLookupTable(),
                         &icmp_header.code, out_err)) {
        return false;
      }
      break;
      
    case ICMP_PARAMETERPROB:
      if (!ExpectNextEnumProperty(reader, "code", GetIcmpBadIpHeaderCodeNames(),
                         &icmp_header.code, out_err)) {
        return false;
      }
      break;
      
    default:
      if (!ExpectNextProperty<uint8_t>(reader, "code", &icmp_header.code, out_err))
        return false;
      break;
  }
  
    switch (icmp_header.type) {
    case ICMP_ECHO: 
    case ICMP_ECHOREPLY:
    case ICMP_TIMESTAMP:
    case ICMP_TIMESTAMPREPLY:
    case ICMP_ADDRESS:
    case ICMP_ADDRESSREPLY:
      if (!ExpectNextProperty<uint16_t>(reader, "identifier", 
              &icmp_header.un.echo.id, out_err)) {
        return false;
      }
      if (!ExpectNextProperty<uint16_t>(reader, "sequenceNumber", 
              &icmp_header.un.echo.sequence, out_err)) {
        return false;
      }
      break;
      
    case ICMP_DEST_UNREACH:
      if (!ExpectNextProperty<uint16_t>(reader, "nextHopMtu", 
            &icmp_header.un.frag.mtu, out_err)) {
        return false;
      }
      break;
      
    case ICMP_REDIRECT:
      if (!ExpectNextPropertyName(reader, "gateway", out_err))
        return false;
      if (!ExpectIp4Value(reader, &icmp_header.un.gateway, out_err)) {
        return false;
      }
      break;
  }
  
  if (!ExpectNextJsonToken(reader, JSON_EndObject, out_err))
    return false;
  
  AppendVectorU8(dest_buffer, &icmp_header, sizeof(icmp_header));
  
  return true;
}

static void Add(const void* header, size_t size, uint32_t* result) {
  const uint16_t* p = static_cast<const uint16_t*>(header);
  const uint16_t* end = reinterpret_cast<const uint16_t*>(
      static_cast<const uint8_t*>(header) + (size/2*2));
  
  for (; p < end; p++) {
    *result += htons(*p);
  }
  
  // if there is an odd number of bytes, we need to zero-pad
  // the last byte
  if (size & 0x01) {
    *result += static_cast<const uint8_t*>(header)[size - 1] << 8;
  }
}

static uint16_t ComputeIpChecksum(const void* pseudo_header, size_t pseudo_size,
                           const void* header, size_t size) {
  uint32_t result = 0;
  Add(pseudo_header, pseudo_size, &result);
  Add(header, size, &result);
  return htons(static_cast<uint16_t>(~((result & 0xffff) + (result >> 16))));
}

static uint16_t ComputeIpChecksum(const void* header, size_t size) {
  uint32_t result = 0;
  Add(header, size, &result);
  return htons(static_cast<uint16_t>(~((result & 0xffff) + (result >> 16))));
}

struct pseudo_header {
  uint32_t source;
  uint32_t dest;
  uint8_t padding;
  uint8_t protocol;
  uint16_t length;
};

bool DeserializePacket(JsonReader* reader, vector<uint8_t>* dest_buffer, string* out_err) {
  if (!ExpectCurrentJsonToken(reader, JSON_StartObject, out_err))
    return false;
  if (!ExpectNextPropertyName(reader, "ip", out_err))
    return false;

  if (!DeserializeIp4Header(reader, dest_buffer, out_err))
    return false;
  
  const iphdr* ip_header = reinterpret_cast<const iphdr*>(&(*dest_buffer)[0]);
  switch (ip_header->protocol) {
    case kProtocolTcp:
      if (!ExpectNextPropertyName(reader, "tcp", out_err))
        return false;
      if (!DeserializeTcpHeader(reader, dest_buffer, out_err))
        return false;
      break;
      
    case kProtocolUdp:
      if (!ExpectNextPropertyName(reader, "udp", out_err))
        return false;
      if (!DeserializeUdpHeader(reader, dest_buffer, out_err))
        return false;
      break;
      
    case kProtocolIcmp:
      if (!ExpectNextPropertyName(reader, "icmp", out_err))
        return false;
      if (!DeserializeIcmpHeader(reader, dest_buffer, out_err))
        return false;
      break;
  }
  
  if (!ExpectNextJsonToken(reader, out_err))
    return false;
  
  if (reader->token_type() != JSON_EndObject) {
    if (!ExpectCurrentPropertyName(reader, "data", out_err))
      return false;
    if (!ExpectNextJsonToken(reader, JSON_StartObject, out_err))
      return false;
    if (!ExpectNextPropertyName(reader, "type", out_err))
      return false;
    if (!ExpectNextJsonToken(reader, JSON_String, out_err))
      return false;
    
    string data_type = reader->str_value();
    if (data_type != "text" && data_type != "hex") {
      return Error(out_err, "data type must be 'text' or 'hex', was '%s'", 
                  data_type.c_str());
    }
    
    if (!ExpectNextPropertyName(reader, "data", out_err))
      return false;
    if (!ExpectNextJsonToken(reader, JSON_StartArray, out_err))
      return false;
    
    string data_str;
    while (true) {
      if (!ExpectNextJsonToken(reader, out_err))
        return false;
      
      if (reader->token_type() == JSON_EndArray)
        break;
      
      if (reader->token_type() != JSON_String) {
        return Error(out_err, "Unexpected token '%s', expected 'String'", 
                    GetJsonTokenName(reader->token_type()));
      }
      
      data_str.append(reader->str_value());
    }
    if (data_type == "text") {
      AppendVectorU8(dest_buffer, data_str.c_str(), data_str.size());
    } else if (data_type == "hex") {
      ParseHex(data_str.c_str(), dest_buffer);
    }
    
    if (!ExpectNextJsonToken(reader, JSON_EndObject, out_err))
      return false;
    if (!ExpectNextJsonToken(reader, JSON_EndObject, out_err))
      return false;
  }
  

  
  const size_t packet_size = dest_buffer->size();
  iphdr* ip_hdr = reinterpret_cast<iphdr*>(&(*dest_buffer)[0]);
  ip_hdr->tot_len = htons(static_cast<uint8_t>(packet_size));
  size_t ip_header_size = ip_hdr->ihl * 4;
  
  ip_hdr->check = ComputeIpChecksum(ip_hdr, ip_header_size);
  
  void* sub_hdr = &(*dest_buffer)[ip_header_size];
  
  switch (ip_hdr->protocol) {
    case kProtocolIcmp: {
      icmphdr* icmp_header = static_cast<icmphdr*>(sub_hdr);
      icmp_header->checksum = ComputeIpChecksum(sub_hdr, 
                                                packet_size - ip_header_size);
      break;
    }
    
    case kProtocolTcp:
    case kProtocolUdp: {
      pseudo_header pseudo;
      assert(sizeof(pseudo) == 12);
      pseudo.source = ip_hdr->saddr;
      pseudo.dest = ip_hdr->daddr;
      pseudo.padding = 0;
      pseudo.protocol = ip_hdr->protocol;
      pseudo.length = htons(static_cast<uint16_t>(packet_size - ip_header_size));
      uint16_t checksum = ComputeIpChecksum(&pseudo, sizeof(pseudo), sub_hdr, 
                                            packet_size - ip_header_size);
      
      if (ip_hdr->protocol == kProtocolUdp) {
        udphdr* udp_header = static_cast<udphdr*>(sub_hdr);
        udp_header->len = htons(static_cast<uint16_t>(packet_size - ip_header_size));
        udp_header->check = checksum;
      } else {
        tcphdr* tcp_header = static_cast<tcphdr*>(sub_hdr);
        tcp_header->check = checksum;
      }
      break;
    }
  }
  
  return true;
}

}