add_library(learning_video_local_transport_lib STATIC
            transport_unit.cpp udp_loopback.cpp loopback_artifact.cpp)
target_include_directories(learning_video_local_transport_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(learning_video_local_transport main.cpp)
target_include_directories(learning_video_local_transport
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../encode)
target_link_libraries(learning_video_local_transport
                      PRIVATE learning_video_local_transport_lib
                              learning_video_encode_lib)
