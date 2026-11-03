enable_language(OBJCXX)

add_library(learning_video_render_lib STATIC render_frames.cpp)
target_include_directories(learning_video_render_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(learning_video_render main.mm)
target_compile_options(learning_video_render PRIVATE -fobjc-arc)
target_link_libraries(
  learning_video_render
  PRIVATE learning_video_render_lib "-framework AppKit" "-framework CoreImage"
          "-framework CoreVideo" "-framework CoreGraphics" "-framework Foundation")
