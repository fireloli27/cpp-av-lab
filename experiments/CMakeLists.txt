cmake_minimum_required(VERSION 3.20)
project(learning_experiments LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_subdirectory(harness/placeholder)
add_subdirectory(audio/capture)
add_subdirectory(audio/encode)
add_subdirectory(audio/local-transport)
add_subdirectory(audio/decode)
add_subdirectory(audio/render)
add_subdirectory(video/capture)
add_subdirectory(video/encode)
add_subdirectory(video/local-transport)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
