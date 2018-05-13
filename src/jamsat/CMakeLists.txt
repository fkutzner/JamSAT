find_package(ZLIB REQUIRED)

# The actual frontend code is in the static library jamsatFrontend => can
# "mock" the IPASIR/JamSAT implementation for testing
add_library(jamsatFrontend STATIC
  JamSAT.h
  JamSAT.cpp
  Parser.h
  Parser.cpp)
jamsat_configure_target(jamsatFrontend)

get_property(jamcore_include_dirs TARGET jamcore PROPERTY INCLUDE_DIRECTORIES)
target_include_directories(jamsatFrontend PRIVATE ${jamcore_include_dirs})
target_include_directories(jamsatFrontend PRIVATE ${ZLIB_INCLUDE_DIRS})
target_link_libraries(jamsatFrontend ${ZLIB_LIBRARIES})
set_property(TARGET jamsatFrontend PROPERTY FOLDER "JamSAT Frontend")

add_executable(jamsat Main.cpp)
jamsat_configure_target(jamsat)
set_property(TARGET jamsat PROPERTY FOLDER "JamSAT Frontend")


target_link_libraries(jamsat jamsatFrontend jamcore)
