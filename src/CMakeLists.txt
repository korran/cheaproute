
include_directories(${cheaproute_SOURCE_DIR}/src)

add_executable(cheaproute 
  main.cc
  netlink.cc)

target_link_libraries(cheaproute cheaproute-base ev)

add_subdirectory(base)

add_subdirectory(test_util)