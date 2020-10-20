# c++11 required
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(../../)
include_directories(../)
add_definitions(-D__STDC_FORMAT_MACROS)
set(field_trial_FILES
field_trial.cc
)
add_library(field_trial SHARED ${field_trial_FILES})
target_link_libraries(field_trial system_wrappers )
