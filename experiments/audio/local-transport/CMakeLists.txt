add_library(learning_audio_local_transport_lib STATIC
            transport_unit.cpp
            udp_loopback.cpp
            loopback_artifact.cpp
            read_opus_caf.cpp)
target_include_directories(learning_audio_local_transport_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(learning_audio_local_transport_lib
                      PUBLIC "-framework AudioToolbox" "-framework CoreFoundation")

add_executable(learning_audio_local_transport main.cpp)
target_link_libraries(learning_audio_local_transport
                      PRIVATE learning_audio_local_transport_lib)
