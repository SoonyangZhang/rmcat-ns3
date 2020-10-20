# c++11 required
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(../../../)
include_directories(../../)
set(bitrate_controller_FILES
bitrate_controller_impl.cc
send_side_bandwidth_estimation.cc
)
add_library(bitrate_controller SHARED ${bitrate_controller_FILES})
target_link_libraries(bitrate_controller system_wrappers rtc_base pacing)
