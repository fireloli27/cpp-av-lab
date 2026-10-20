include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

add_executable(learning_tests test_placeholder_artifact.cpp)
target_link_libraries(learning_tests PRIVATE learning_placeholder_lib
                                             Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(learning_tests)
