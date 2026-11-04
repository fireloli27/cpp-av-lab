add_library(learning_video_encode_lib STATIC vau.cpp load_capture.cpp encode_vt.cpp)
target_include_directories(learning_video_encode_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  learning_video_encode_lib
  PUBLIC "-framework VideoToolbox" "-framework CoreMedia" "-framework CoreVideo"
         "-framework CoreFoundation")

add_executable(learning_video_encode main.cpp)
target_link_libraries(learning_video_encode PRIVATE learning_video_encode_lib)
