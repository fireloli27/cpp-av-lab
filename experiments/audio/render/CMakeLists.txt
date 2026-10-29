add_library(learning_audio_render_lib STATIC render_pcm.cpp play_queue.cpp)
target_include_directories(learning_audio_render_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(learning_audio_render_lib
                      PUBLIC "-framework AudioToolbox" "-framework CoreAudio"
                             "-framework CoreFoundation")

add_executable(learning_audio_render main.cpp)
target_link_libraries(learning_audio_render PRIVATE learning_audio_render_lib)
