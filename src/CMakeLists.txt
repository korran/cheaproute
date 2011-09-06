
include_directories(${cheaproute_SOURCE_DIR}/src)

add_executable(cheaproute 
  main.cc)

target_link_libraries(cheaproute cheaproute-net cheaproute-base ev)

add_subdirectory(base)
add_subdirectory(net)
add_subdirectory(test_util)
