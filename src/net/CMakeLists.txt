
add_library(cheaproute-net
  ip_address.cc
  netlink.cc)

add_executable(cheaproute-net-tests
               ip_address_test.cc)

add_test(cheaproute-net-tests cheaproute-net-tests)

target_link_libraries(cheaproute-net-tests 
                      cheaproute-net
                      cheaproute-base
                      gtest_main)