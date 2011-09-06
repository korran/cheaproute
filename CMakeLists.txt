cmake_minimum_required(VERSION 2.6)

project(cheaproute)

if(NOT EXISTS ${CMAKE_BINARY_DIR}/gtest-1.6.0.zip)
  file(DOWNLOAD http://googletest.googlecode.com/files/gtest-1.6.0.zip 
       ${CMAKE_BINARY_DIR}/gtest-1.6.0.zip)
endif(NOT EXISTS ${CMAKE_BINARY_DIR}/gtest-1.6.0.zip)

if(NOT EXISTS ${CMAKE_BINARY_DIR}/gtest-1.6.0)
  execute_process(
    COMMAND unzip gtest-1.6.0.zip
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif(NOT EXISTS ${CMAKE_BINARY_DIR}/gtest-1.6.0)

add_subdirectory(${CMAKE_BINARY_DIR}/gtest-1.6.0)

include_directories(${CMAKE_BINARY_DIR}/gtest-1.6.0/include
)

add_definitions(-Wall -Wstrict-aliasing=2 -Wconversion -Wredundant-decls -Werror -Wclobbered -Wempty-body -Wuninitialized
                -D__STDC_FORMAT_MACROS
                -D__STDC_LIMIT_MACROS)
enable_testing()
add_subdirectory(src)





