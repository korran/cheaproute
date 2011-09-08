
#include "net/json_packet.h"
#include "gtest/gtest.h"

#include "base/json_reader.h"
#include <test_util/json.h>

#include <arpa/inet.h>

namespace cheaproute {
  

static vector<uint8_t> ParseHex(const char* str) {
  vector<uint8_t> result;
  ParseHex(str, &result);
  return result;
}
  
TEST(JsonPacketTest, SerializeTcpSyn) {
  vector<uint8_t> packet = ParseHex(
    "4500003cb68a40004006ad96c0a8017cc0a80178cda2005067853c820000"
    "0000a0023908d6c90000020405b40402080a00508a080000000001030307");

  JsonWriterFixture json;
  SerializePacket(json.writer(), &packet[0], packet.size());
  json.AssertContents("{"
                        "\"ip\":{"
                          "\"version\":4,"
                          "\"tos\":0,"
                          "\"id\":46730,"
                          "\"flags\":[\"DF\"],"
                          "\"fragmentOffset\":0,"
                          "\"ttl\":64,"
                          "\"protocol\":\"TCP\","
                          "\"source\":\"192.168.1.124\","
                          "\"destination\":\"192.168.1.120\""
                        "},"
                        "\"tcp\":{"
                          "\"sourcePort\":52642,"
                          "\"destPort\":80,"
                          "\"seqNumber\":1736785026,"
                          "\"flags\":[\"SYN\"],"
                          "\"windowSize\":14600,"
                          "\"options\":[[\"maxSegmentSize\",1460],[\"sackPermitted\"],"
                                       "[\"timestamp\",5278216,0],[\"NOP\"],"
                                       "[\"windowScale\",7]"
                          "]"
                        "}"
                      "}");  
}

TEST(JsonPacketTest, SerializeTcpPacketWithData) {
  vector<uint8_t> packet = ParseHex(
      "45000081066a400040065d46c0a8017d480ecc939a920050610af70b85da5e53801800"
      "73090e00000101080a0133f17603175625474554202f20485454502f312e310d0a4163"
      "636570743a202a2f2a0d0a486f73743a207777772e676f6f676c652e636f6d0d0a436f"
      "6e6e656374696f6e3a204b6565702d416c6976650d0a0d0a");
  
  JsonWriterFixture json;
  SerializePacket(json.writer(), &packet[0], packet.size());
  json.AssertContents("{"
                        "\"ip\":{"
                          "\"version\":4,"
                          "\"tos\":0,"
                          "\"id\":1642,"
                          "\"flags\":[\"DF\"],"
                          "\"fragmentOffset\":0,"
                          "\"ttl\":64,"
                          "\"protocol\":\"TCP\","
                          "\"source\":\"192.168.1.125\","
                          "\"destination\":\"72.14.204.147\""
                        "},"
                        "\"tcp\":{"
                          "\"sourcePort\":39570,"
                          "\"destPort\":80,"
                          "\"seqNumber\":1628108555,"
                          "\"ackNumber\":2245680723,"
                          "\"flags\":[\"ACK\",\"PSH\"],"
                          "\"windowSize\":115,"
                          "\"options\":[[\"NOP\"],[\"NOP\"],[\"timestamp\",20181366,51861029]]"
                        "},"
                        "\"data\":{\"type\":\"text\",\"data\":["
                            "\"GET / HTTP/1.1\\r\\n\","
                            "\"Accept: */*\\r\\n\","
                            "\"Host: www.google.com\\r\\n\","
                            "\"Connection: Keep-Alive\\r\\n\","
                            "\"\\r\\n\""
                          "]"
                        "}"
                      "}");
}

TEST(JsonPacketTest, SerializeUdpPacket) {
  vector<uint8_t> packet = ParseHex(
      "4500003cd3f400004011d480c0a8018408080808c9e000350028d27545350100000100"
      "00000000000377777706676f6f676c6503636f6d0000010001");
  
  JsonWriterFixture json;
  SerializePacket(json.writer(), &packet[0], packet.size());
  json.AssertContents("{"
                        "\"ip\":{"
                          "\"version\":4,"
                          "\"tos\":0,"
                          "\"id\":54260,"
                          "\"flags\":[],"
                          "\"fragmentOffset\":0,"
                          "\"ttl\":64,"
                          "\"protocol\":\"UDP\","
                          "\"source\":\"192.168.1.132\","
                          "\"destination\":\"8.8.8.8\""
                        "},"
                        "\"udp\":{"
                          "\"sourcePort\":51680,"
                          "\"destPort\":53"
                        "},"
                        "\"data\":{\"type\":\"hex\",\"data\":["
                            "\"45 35 01 00 00 01 00 00  00 00 00 00 03 77 77 77\","
                            "\"06 67 6f 6f 67 6c 65 03  63 6f 6d 00 00 01 00 01\""
                          "]"
                        "}"
                      "}");
}

TEST(JsonPacketTest, SerializeIcmpPacket) {
  vector<uint8_t> packet = ParseHex(
      "45000054000040004001686dc0a8018408080808080066b946490001"
      "b044654e000000006c960a0000000000101112131415161718191a1b"
      "1c1d1e1f202122232425262728292a2b2c2d2e2f3031323334353637");
  
  JsonWriterFixture json;
  SerializePacket(json.writer(), &packet[0], packet.size());
  json.AssertContents("{"
                        "\"ip\":{"
                          "\"version\":4,"
                          "\"tos\":0,"
                          "\"id\":0,"
                          "\"flags\":[\"DF\"],"
                          "\"fragmentOffset\":0,"
                          "\"ttl\":64,"
                          "\"protocol\":\"ICMP\","
                          "\"source\":\"192.168.1.132\","
                          "\"destination\":\"8.8.8.8\""
                        "},"
                        "\"icmp\":{"
                          "\"type\":\"echoRequest\","
                          "\"code\":0,"
                          "\"identifier\":17993,"
                          "\"sequenceNumber\":1"
                        "},"
                        "\"data\":{\"type\":\"hex\",\"data\":["
                            "\"b0 44 65 4e 00 00 00 00  6c 96 0a 00 00 00 00 00\","
                            "\"10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f\","
                            "\"20 21 22 23 24 25 26 27  28 29 2a 2b 2c 2d 2e 2f\","
                            "\"30 31 32 33 34 35 36 37\""
                          "]"
                        "}"
                      "}");
}

static vector<uint8_t> DeserializePacketOrFail(const char* json) {
  
  JsonReader reader(CreateBufferedInputStream(json));
  EXPECT_TRUE(reader.Next());
  
  vector<uint8_t> packet_data;
  string error_string;
  if (!DeserializePacket(&reader, &packet_data, &error_string))
    EXPECT_TRUE(false) << "Packet serialization failed with message: " << error_string;
  
  return packet_data;
}

static void AssertBinaryEqual(const vector<uint8_t>& a, const vector<uint8_t>& b) {
  ASSERT_EQ(FormatHex(a.empty() ? NULL : &a[0], a.size()), 
            FormatHex(b.empty() ? NULL : &b[0], b.size()));
}

TEST(JsonPacketTest, DeserializeTcpPacket) {
  vector<uint8_t> packet = DeserializePacketOrFail("{"
    "\"ip\":{"
      "\"version\":4,"
      "\"tos\":0,"
      "\"id\":1642,"
      "\"flags\":[\"DF\"],"
      "\"fragmentOffset\":0,"
      "\"ttl\":64,"
      "\"protocol\":\"TCP\","
      "\"source\":\"192.168.1.125\","
      "\"destination\":\"72.14.204.147\""
    "},"
    "\"tcp\":{"
      "\"sourcePort\":39570,"
      "\"destPort\":80,"
      "\"seqNumber\":1628108555,"
      "\"ackNumber\":2245680723,"
      "\"flags\":[\"ACK\",\"PSH\"],"
      "\"windowSize\":115,"
      "\"options\":[[\"NOP\"],[\"NOP\"],[\"timestamp\",20181366,51861029]]"
    "},"
    "\"data\":{\"type\":\"text\",\"data\":["
        "\"GET / HTTP/1.1\\r\\n\","
        "\"Accept: */*\\r\\n\","
        "\"Host: www.google.com\\r\\n\","
        "\"Connection: Keep-Alive\\r\\n\","
        "\"\\r\\n\""
      "]"
    "}"
  "}");
  
  AssertBinaryEqual(ParseHex(
      "45000081066a400040065d46c0a8017d480ecc939a920050610af70b85da5e53801800"
      "73090e00000101080a0133f17603175625474554202f20485454502f312e310d0a4163"
      "636570743a202a2f2a0d0a486f73743a207777772e676f6f676c652e636f6d0d0a436f"
      "6e6e656374696f6e3a204b6565702d416c6976650d0a0d0a"), packet); 
}

TEST(JsonPacketTest, DeserializeUdpPacket) {
  vector<uint8_t> packet = DeserializePacketOrFail("{"
                        "\"ip\":{"
                          "\"version\":4,"
                          "\"tos\":0,"
                          "\"id\":54260,"
                          "\"flags\":[],"
                          "\"fragmentOffset\":0,"
                          "\"ttl\":64,"
                          "\"protocol\":\"UDP\","
                          "\"source\":\"192.168.1.132\","
                          "\"destination\":\"8.8.8.8\""
                        "},"
                        "\"udp\":{"
                          "\"sourcePort\":51680,"
                          "\"destPort\":53"
                        "},"
                        "\"data\":{\"type\":\"hex\",\"data\":["
                            "\"45 35 01 00 00 01 00 00  00 00 00 00 03 77 77 77\","
                            "\"06 67 6f 6f 67 6c 65 03  63 6f 6d 00 00 01 00 01\""
                          "]"
                        "}"
                      "}");
  
  AssertBinaryEqual(ParseHex(
    "4500003cd3f400004011d480c0a8018408080808c9e0003500288f6f45350100000100"
    "00000000000377777706676f6f676c6503636f6d0000010001"), packet);
}

void TestJsonRoundTrip(const char* json) {

  vector<uint8_t> packet_data = DeserializePacketOrFail(json);

  
  JsonWriterFixture fixture;
  SerializePacket(fixture.writer(), &packet_data[0], packet_data.size());
  fixture.AssertContents(json);
}



TEST(JsonPacketTest, RoundTripTcpPacket) {
  TestJsonRoundTrip(
    "{"
      "\"ip\":{"
        "\"version\":4,"
        "\"tos\":0,"
        "\"id\":1642,"
        "\"flags\":[\"DF\"],"
        "\"fragmentOffset\":0,"
        "\"ttl\":64,"
        "\"protocol\":\"TCP\","
        "\"source\":\"192.168.1.125\","
        "\"destination\":\"72.14.204.147\""
      "},"
      "\"tcp\":{"
        "\"sourcePort\":39570,"
        "\"destPort\":80,"
        "\"seqNumber\":1628108555,"
        "\"ackNumber\":2245680723,"
        "\"flags\":[\"ACK\",\"PSH\"],"
        "\"windowSize\":115"
      "},"
      "\"data\":{\"type\":\"text\",\"data\":["
          "\"GET / HTTP/1.1\\r\\n\","
          "\"Accept: */*\\r\\n\","
          "\"Host: www.google.com\\r\\n\","
          "\"Connection: Keep-Alive\\r\\n\","
          "\"\\r\\n\""
        "]"
      "}"
    "}");
}

TEST(JsonPacketTest, RoundTripUdpPacket) {
  TestJsonRoundTrip(
    "{"
      "\"ip\":{"
        "\"version\":4,"
        "\"tos\":0,"
        "\"id\":54260,"
        "\"flags\":[],"
        "\"fragmentOffset\":0,"
        "\"ttl\":64,"
        "\"protocol\":\"UDP\","
        "\"source\":\"192.168.1.132\","
        "\"destination\":\"8.8.8.8\""
      "},"
      "\"udp\":{"
        "\"sourcePort\":51680,"
        "\"destPort\":53"
      "},"
      "\"data\":{\"type\":\"hex\",\"data\":["
          "\"45 35 01 00 00 01 00 00  00 00 00 00 03 77 77 77\","
          "\"06 67 6f 6f 67 6c 65 03  63 6f 6d 00 00 01 00 01\""
        "]"
      "}"
    "}");
}

TEST(JsonPacketTest, RoundTripIcmpPacket) {
  TestJsonRoundTrip(
    "{"
      "\"ip\":{"
        "\"version\":4,"
        "\"tos\":0,"
        "\"id\":0,"
        "\"flags\":[\"DF\"],"
        "\"fragmentOffset\":0,"
        "\"ttl\":64,"
        "\"protocol\":\"ICMP\","
        "\"source\":\"192.168.1.132\","
        "\"destination\":\"8.8.8.8\""
      "},"
      "\"icmp\":{"
        "\"type\":\"echoRequest\","
        "\"code\":0,"
        "\"identifier\":17993,"
        "\"sequenceNumber\":1"
      "},"
      "\"data\":{\"type\":\"hex\",\"data\":["
          "\"b0 44 65 4e 00 00 00 00  6c 96 0a 00 00 00 00 00\","
          "\"10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f\","
          "\"20 21 22 23 24 25 26 27  28 29 2a 2b 2c 2d 2e 2f\","
          "\"30 31 32 33 34 35 36 37\""
        "]"
      "}"
    "}");
}

// TODO: Need test cases to exercise all the crazy branches in the parsing code

}