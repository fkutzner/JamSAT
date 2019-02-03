# Using JamSAT

## Installed Artifacts

### Binaries

* `jamsat` rsp. `jamsat.exe` is JamSAT's frontend, for solving
  DIMACS-formatted SAT problem instances. The problem instances
  can be provided as plain-text or gzip-compressed files.
  Run `jamsat --help` for a description of its parameters and
  output rsp. return values.

* Depending on the setting of `JAMSAT_BUILD_STATIC_LIB`, one of
    * A static library containing the JamSAT solver, implementing
      the IPASIR interface. Depending on your platform, this file
      is named `libjamsats.a` or `jamsats.lib`.

    * A dynamic library containing the JamSAT solver, implementing
      the IPASIR interface. Depending on your platform, this file
      is named `libjamsatd.so`, `libjamsatd.dylib` or `jamsatd.dll`.

### Header files

* `jamsat/JamSatIpasir.h` contains the IPASIR interface declarations
  for the JamSAT solver library. When using JamSAT as a shared library,
  set the `JAMSAT_SHARED_LIB` preprocessor flag before including this
  header file. Clients using CMake are not required to deal with this
  flag directly - see below.
  See http://github.com/biotomas/ipasir for details about the IPASIR
  interface.

## Adding JamSAT to CMake Projects

See the `examples/` directory for examples illustrating how to use
JamSAT in a client project.

### JamSAT packages

To find a shared-library JamSAT installation, use
`find_package(JamSATShared REQUIRED)`. Similarly, to find a static-library
JamSAT installation, use `find_package(JamSATStatic REQUIRED)`.

### Linking to JamSAT

Both `find_package` invocations described above provide a target
`jamcore` representing the JamSAT solver library. The `jamcore` target
carries interface definitions: include directories and preprocessor
flags (e.g. `JAMSAT_SHARED_LIB`) are automatically set up for targets
linking to `jamcore`.

**Note:** Linking to `jamcore` only causes preprocessor flags related to
the JamSAT API as well as include search paths to be added to the client
library's target. In particular, language-related compiler flags are not
modified. However, you can insulate your code by creating a thin wrapper
module and linking `jamcore` against that module's target, with `PRIVATE`
scope.
