# The JamSAT Repository

## Directory Layout

- `deps/` - dependency submodules
  - `minisat/` - MiniSat (test-only dependency)
  - `state_ptr/`
- `etc/` - miscellaneous files
  - `etc/cmake` - CMake modules used for building JamSAT
- `examples/` - example JamSAT client programs
- `include/` - public library headers. Clients of a library should only use header files contained in this directory.
  - `libjamsat/`
  - `libjamfrontend/`
- `lib/` - library implementations
 - `libjamsat/` - the JamSAT library, exporting an IPASIR interface
 - `libjamfrontend/` - the (static) JamSAT frontend library, implementing the standalone JamSAT frontend functionality
- `testsrc/`
  - `libjamsat/` - tests for `libjamsat`
  - `libjamfrontend/` - tests for `libjamfrontend`
  - `toolbox/` - test utilities (CNF generators, ...)
- `tools/`
  - `jamsat/` - implementation of the standalone JamSAT executable
