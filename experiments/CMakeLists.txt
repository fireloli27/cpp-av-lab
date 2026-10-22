cmake_minimum_required(VERSION 3.20)
project(learning_experiments LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_subdirectory(harness/placeholder)
add_subdirectory(audio/capture)
add_subdirectory(audio/encode)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
