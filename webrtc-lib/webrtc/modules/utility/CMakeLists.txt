# c++11 required
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(../../../)
include_directories(../../)
set(utility_FILES
source/process_thread_impl.cc
)
add_library(utility STATIC ${utility_FILES})
