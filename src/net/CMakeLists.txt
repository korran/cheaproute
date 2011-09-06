
add_library(cheaproute-net
  ip_address.cc
  json_packet.cc
  netlink.cc
  tun_interface.cc)

add_executable(cheaproute-net-tests
               json_packet_test.cc
               ip_address_test.cc)

add_test(cheaproute-net-tests cheaproute-net-tests)

target_link_libraries(cheaproute-net-tests 
                      cheaproute-net
                      cheaproute-test-util
                      cheaproute-base
                      gtest_main)
