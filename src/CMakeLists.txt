
include_directories(${cheaproute_SOURCE_DIR}/src)

add_executable(cheaproute 
  main.cc)

target_link_libraries(cheaproute cheaproute-base cheaproute-net ev)

add_subdirectory(base)

add_subdirectory(test_util)
add_subdirectory(net)