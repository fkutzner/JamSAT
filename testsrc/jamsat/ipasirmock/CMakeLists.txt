add_library(ipasirmock OBJECT IpasirMock.cpp IpasirMock.h)
jamsat_configure_target(ipasirmock)

get_property(jamcore_include_dirs TARGET jamcore PROPERTY INCLUDE_DIRECTORIES)
target_include_directories(ipasirmock PRIVATE ${jamcore_include_dirs})
