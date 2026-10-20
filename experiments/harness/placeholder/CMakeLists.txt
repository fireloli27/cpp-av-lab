add_library(learning_placeholder_lib STATIC placeholder_artifact.cpp)
target_include_directories(learning_placeholder_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(learning_placeholder main.cpp)
target_link_libraries(learning_placeholder PRIVATE learning_placeholder_lib)
