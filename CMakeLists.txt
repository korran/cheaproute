project(cheaproute)

add_executable(cheaproute 
  main.cc)

target_link_libraries(cheaproute cheaproute-base ev)
add_subdirectory(base)