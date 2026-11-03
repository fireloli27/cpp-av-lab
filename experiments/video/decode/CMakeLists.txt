enable_language(OBJCXX)

add_library(learning_video_decode_lib STATIC decode_vau.mm)
target_include_directories(learning_video_decode_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(learning_video_decode_lib PRIVATE -fobjc-arc)
target_link_libraries(
  learning_video_decode_lib
  PUBLIC learning_video_encode_lib
         "-framework VideoToolbox" "-framework CoreMedia" "-framework CoreVideo"
         "-framework CoreImage" "-framework CoreGraphics" "-framework ImageIO"
         "-framework UniformTypeIdentifiers" "-framework Foundation")

add_executable(learning_video_decode main.cpp)
target_link_libraries(learning_video_decode PRIVATE learning_video_decode_lib)
