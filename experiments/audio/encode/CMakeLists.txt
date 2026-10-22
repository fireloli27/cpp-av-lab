add_library(learning_audio_encode_lib STATIC encode_caf.cpp)
target_include_directories(learning_audio_encode_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(learning_audio_encode_lib
                      PUBLIC "-framework AudioToolbox" "-framework CoreAudio"
                             "-framework CoreFoundation")

add_executable(learning_audio_encode main.cpp)
target_link_libraries(learning_audio_encode PRIVATE learning_audio_encode_lib)
