

add_library(cheaproute-base
  broadcaster.cc
  common.cc
  event_loop.cc
  file_descriptor.cc
  json_reader.cc
  stream.cc)

add_executable(cheaproute-base-tests
               json_reader_test.cc
               stream_test.cc)

add_test(cheaproute-base-tests cheaproute-base-tests)

target_link_libraries(cheaproute-base-tests 
                      cheaproute-base
                      cheaproute-test-util
                      gtest_main)

