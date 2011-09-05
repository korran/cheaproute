#include "base/common.h"

#include "gtest/gtest.h"
#include "test_util/stream.h"


namespace cheaproute {
 
 
TEST(FormatHex,  SingleLine) {
  uint8_t data[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                     0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87 };
  
  ASSERT_EQ("01 23 45 67 89 ab cd ef  f0 e1 d2 c3 b4 a5 96 87", 
            FormatHex(data, sizeof(data)));
}

TEST(FormatHex,  MultiLine) {
  uint8_t data[] = { 0x8a, 0x1b, 0x5e, 0x7a, 0x83, 0x5f, 0x12, 0xa3,
                     0x92, 0x23, 0x5a, 0xb4, 0xac, 0x52, 0x5e, 0x1a,
                     0x1b, 0x5e, 0x8d, 0x41, 0xd7, 0xcc, 0xfa, 0x43,
                     0x2f, 0x4a, 0x90, 0x4d, 0xa3, 0xa1, 0xef, 0x73,
                     0x43};

  ASSERT_EQ("8a 1b 5e 7a 83 5f 12 a3  92 23 5a b4 ac 52 5e 1a\n"
            "1b 5e 8d 41 d7 cc fa 43  2f 4a 90 4d a3 a1 ef 73\n"
            "43", 
            FormatHex(data, sizeof(data)));
}

TEST(ParseHex, Simple) {
  const uint8_t expected_result[]  = 
    {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    
  vector<uint8_t> data;
  ASSERT_TRUE(ParseHex("0123456789abcdef", &data));
  ASSERT_EQ(sizeof(expected_result), data.size());
  ASSERT_EQ(vector<uint8_t>(expected_result, 
                            expected_result + sizeof(expected_result)), 
            data);
}

TEST(ParseHex, Whitespace) {
  const uint8_t expected_result[]  = 
    {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    
  vector<uint8_t> data;
  ASSERT_TRUE(ParseHex("01 23 45\n67   89 AB CD\tEF", &data));
  ASSERT_EQ(sizeof(expected_result), data.size());
  ASSERT_EQ(vector<uint8_t>(expected_result, 
                            expected_result + sizeof(expected_result)), 
            data);
}

TEST(ParseHex, Errors) {
  vector<uint8_t> data;
  ASSERT_FALSE(ParseHex("0x", &data));
  ASSERT_FALSE(ParseHex("d", &data));
  ASSERT_FALSE(ParseHex("d e", &data));
  ASSERT_FALSE(ParseHex("hello", &data));
}

}
