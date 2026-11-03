enable_language(OBJCXX)

add_library(learning_video_capture_lib STATIC nv12.cpp capture_publish.mm)
target_include_directories(learning_video_capture_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(learning_video_capture_lib PRIVATE -fobjc-arc)
target_link_libraries(
  learning_video_capture_lib
  PUBLIC "-framework CoreVideo" "-framework CoreImage" "-framework CoreGraphics"
         "-framework ImageIO" "-framework UniformTypeIdentifiers"
         "-framework Foundation")

add_executable(learning_video_capture main.mm)
target_compile_options(learning_video_capture PRIVATE -fobjc-arc)
target_link_libraries(
  learning_video_capture
  PRIVATE learning_video_capture_lib "-framework AVFoundation"
          "-framework CoreMedia" "-framework CoreVideo" "-framework Foundation")
