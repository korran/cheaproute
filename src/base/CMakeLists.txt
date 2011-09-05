

add_library(cheaproute-base
  broadcaster.cc
  common.cc
  event_loop.cc
  file_descriptor.cc
  json_reader.cc
  json_writer.cc
  stream.cc)

add_executable(cheaproute-base-tests
               common_test.cc
               json_reader_test.cc
               json_writer_test.cc
               stream_test.cc)

add_test(cheaproute-base-tests cheaproute-base-tests)

target_link_libraries(cheaproute-base-tests 
                      cheaproute-base
                      cheaproute-test-util
                      gtest_main)

