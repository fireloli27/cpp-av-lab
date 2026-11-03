include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.1
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

add_executable(learning_tests test_placeholder_artifact.cpp
                              test_capture_wav.cpp
                              test_encode_caf.cpp
                              test_local_transport.cpp
                              test_decode_wav.cpp
                              test_render_pcm.cpp
                              test_video_capture.cpp
                              test_video_encode.cpp
                              test_video_local_transport.cpp
                              test_video_decode.cpp
                              test_video_render.cpp)
target_link_libraries(learning_tests PRIVATE learning_placeholder_lib
                                             learning_audio_capture_lib
                                             learning_audio_encode_lib
                                             learning_audio_local_transport_lib
                                             learning_audio_decode_lib
                                             learning_audio_render_lib
                                             learning_video_capture_lib
                                             learning_video_encode_lib
                                             learning_video_local_transport_lib
                                             learning_video_decode_lib
                                             learning_video_render_lib
                                             Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(learning_tests)
