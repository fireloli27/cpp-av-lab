add_library(learning_audio_capture_lib STATIC capture_wav.cpp)
target_include_directories(learning_audio_capture_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(learning_audio_capture main.cpp)
target_link_libraries(learning_audio_capture PRIVATE learning_audio_capture_lib
                                                     "-framework AudioToolbox"
                                                     "-framework CoreAudio")
