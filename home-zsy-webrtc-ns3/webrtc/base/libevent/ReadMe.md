libevent should install here.   
And change the CMakeLists.txt under rtc_base:  
include_directories(/the_path_to_libevent_header/libevent/include)   
FIND_LIBRARY(COMM_LIB event "/the_path_to_libevent_lib/libevent/lib" NO_DEFAULT_PATH)   
