# c++11 required
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(../../)
include_directories(../)
add_definitions(-D__STDC_FORMAT_MACROS)
set(system_wrapper_FILES
source/aligned_malloc.cc
source/atomic32.cc
source/clock.cc
source/cpu_features.cc
source/cpu_info.cc
source/event.cc
source/event_timer_posix.cc
source/file_impl.cc
source/rtp_to_ntp_estimator.cc
source/rw_lock.cc
source/rw_lock_posix.cc
source/sleep.cc
source/timestamp_extrapolator.cc
source/metrics_default.cc
)
add_library(system_wrappers SHARED ${system_wrapper_FILES})
target_link_libraries(system_wrappers rtc_base pthread rt)
