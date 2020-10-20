# c++11 required
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(../../../)
include_directories(../../)
set(audio_coding_FILES
audio_network_adaptor/audio_network_adaptor_config.cc
)
add_library(audio_coding SHARED ${audio_coding_FILES})
target_link_libraries(audio_coding  api)
