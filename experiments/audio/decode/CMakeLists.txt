add_library(learning_audio_decode_lib STATIC decode_wav.cpp)
target_include_directories(learning_audio_decode_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(learning_audio_decode_lib
                      PUBLIC "-framework AudioToolbox" "-framework CoreAudio"
                             "-framework CoreFoundation")

add_executable(learning_audio_decode main.cpp)
target_link_libraries(learning_audio_decode PRIVATE learning_audio_decode_lib)
