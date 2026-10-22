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
                              test_encode_caf.cpp)
target_link_libraries(learning_tests PRIVATE learning_placeholder_lib
                                             learning_audio_capture_lib
                                             learning_audio_encode_lib
                                             Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(learning_tests)
