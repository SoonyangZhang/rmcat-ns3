# c++11 required
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(../)
include_directories(../../)
set(api_FILES
optional.cc
rtpparameters.cc
audio_codecs/audio_format.cc
video/video_content_type.cc
)
add_library(api SHARED ${api_FILES})
