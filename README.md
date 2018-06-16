|        Linux        |       Windows       |      Test Coverage       |     SonarSource        |        Codacy             |      Size         |
|:-------------------:|:-------------------:|:------------------------:|:----------------------:|:-------------------------:|:-----------------:|
| [![travisCI][1]][2] | [![appveyor][3]][4] | [![coveralls][5]][6]     | [![sonarsource][7]][8] | [![Codacy Badge][11]][12] | [![tokei][9]][10] |

[1]: https://travis-ci.org/fkutzner/JamSAT.svg?branch=master
[2]: https://travis-ci.org/fkutzner/JamSAT
[3]: https://ci.appveyor.com/api/projects/status/88983cn7gmg91b3s/branch/master?svg=true
[4]: https://ci.appveyor.com/project/fkutzner/jamsat/branch/master
[5]: https://coveralls.io/repos/github/fkutzner/JamSAT/badge.svg?branch=master
[6]: https://coveralls.io/github/fkutzner/JamSAT?branch=master
[7]: https://sonarcloud.io/api/project_badges/measure?project=jamsat&metric=alert_status
[8]: https://sonarcloud.io/dashboard?id=jamsat
[9]: https://tokei.rs/b1/github/fkutzner/JamSAT?category=code
[10]: https://github.com/Aaronepower/tokei#badges
[11]: https://api.codacy.com/project/badge/Grade/9e68cb1a29c94839a5456c6d75d6b6b0
[12]: https://app.codacy.com/app/fkutzner/JamSAT?utm_source=github.com&utm_medium=referral&utm_content=fkutzner/JamSAT&utm_campaign=badger

# JamSAT

JamSAT is a fast, clean incremental
[SAT](https://en.wikipedia.org/wiki/Boolean_satisfiability_problem)
solver implementing the
[IPASIR interface](https://github.com/biotomas/ipasir). Though
not being a descendant of [Minisat](http://minisat.se),
this solver is heavily influenced by Minisat,
[Glucose](http://www.labri.fr/perso/lsimon/glucose/) and
the [Candy Kingdom](https://github.com/Udopia/candy-kingdom).
Hence the marmalade-themed name.

Being a relatively young SAT solver as well as a side
project, JamSAT is yet missing some important
features. The [implementation status](#implementation-status)
section contains a summary of its capabilities.

JamSAT is free software distributed under the [MIT license](LICENSE)
(X11 variant).

## Scope

While JamSAT can be used as a generic SAT solver, its focus lies on
solving problem instances arising from industrial applications such
as model checking, artificial intelligence, circuit equivalence checking
and SMT solving. More
specifically, it is optimized for _agile_ SAT solving: it is designed
to efficiently solve vast numbers of SAT problem instances that are
not very hard individually.

## Supported Platforms and Ecosystems

JamSAT is regularly built and tested using the following
configurations:

| Operating System    | Compilers           |
|---------------------|---------------------|
| GNU/Linux           | clang, gcc          |
| Apple macOS         | clang               |
| Microsoft Windows   | Visual C++ 2017     |

A compiler supporting C++14 is required to build JamSAT.

JamSAT integrates nicely with [CMake](http://cmake.org) projects.
The repository
contains an [example](examples/ipasirclient) showing how to
use JamSAT in client applications. Support for
[pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
is planned, but not yet implemented.


## Building JamSAT

The following prerequisites must be present on your system:
* CMake (at least version 3.2.2)
* a C++ compiler with C++14 support
* [Boost](https://www.boost.org) (at least version 1.57)
* [zlib](https://github.com/madler/zlib)

### Building without tests

To build JamSAT, execute the following commands, with
`<I>` being the directory where JamSAT shall be installed:

```
git clone https://github.com/fkutzner/JamSAT
cd JamSAT
git submodule init
git submodule update
cd ..
mkdir JamSAT-build
cd JamSAT-build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<I> ../jamsat
cmake --build . --target install
```

_Note for Windows builds_: You'll need to pass the path
to Boost and the zlib installation to the first `cmake`
invocation as well. See the [AppVeyor configuration](appveyor.yml)
for a build recipe.

### Building with tests

Building the JamSAT test suite can be enabled by additionally
passing the argument `-DJAMSAT_ENABLE_TESTING=ON` to the first
`cmake` invocation. The tests have an additional dependency
on Minisat (for acceptance test result comparisons). You'll
need to build a Minisat [fork](https://github.com/fkutzner/minisat)
and add the installation directory of that fork to
JamSAT's `CMAKE_PREFIX_PATH`.

To run the test suite, navigate to JamSAT's build directory
(e.g. the `JamSAT-build` created in the script above) and run
`ctest`.


JamSAT supports fuzz testing with
[afl-fuzz](http://lcamtuf.coredump.cx/afl/). To enable
fuzzing, pass `-DJAMSAT_ENABLE_AFL_FUZZER=ON` to the first
`cmake` invocation. You can then fuzz-test JamSAT by
building the target `barecdclsolver_fuzzer-run`. Fuzzing results
will be placed in the directory
`artifacts/bin/barecdclsolver_fuzzer-fuzzer-findings`.

See the [Travis CI build script](etc/TravisBuild.sh) or
[AppVeyor configuration](appveyor.yml) for a more detailed
description of building with testing enabled.

### Build Options

TODO: Describe options that can be passed to JamSAT's CMake
scripts

## Using JamSAT

The JamSAT build produces three artifacts:

* `jamsat` rsp. `jamsat.exe` is JamSAT's frontend, for solving
  DIMACS-formatted SAT problem instances. The problem instances
  can be provided as plain-text or gzip-compressed files.
  Run `jamsat --help` for a description of its parameters and
  output rsp. return values.

* A static library containing the JamSAT solver, implementing
  the IPASIR interface. Depending on your platform, this file
  is named `libjamsats.a` or `jamsats.lib`.

* A dynamic library containing the JamSAT solver, implementing
  the IPASIR interface. Depending on your platform, this file
  is named `libjamsatd.so`, `libjamsatd.dylib` or `jamsatd.dll`.

## Implementation Status

| Capability              | Status                  |
| ----------------------- | :----------------------:|
| Incremental SAT solving | Supported               |
| IPASIR interface        | Mostly supported        |
| Problem simplification  | Partially supported     |
| UNSAT Certificates      | TODO                    |

### Problem Simplification
JamSAT employs inprocessing techniques to simplify the
problem-instance during the solving process, leveraging
lemmas derived from the instance. Currently, JamSAT
implements the following techniques:

* Clause subsumption with hyper-binary resolution
* Self-subsuming resolution with hyper-binary resolution
* Failed Literal Elimination

### IPASIR Implementation Status

| Function | Status |
| -------- | :----: |
| ipasir_signature | Supported |
| ipasir_init | Supported |
| ipasir_release | Supported |
| ipasir_add | Supported |
| ipasir_assume | Supported |
| ipasir_solve | Supported |
| ipasir_val | Supported |
| ipasir_failed | Supported |
| ipasir_set_terminate | Supported |
| ipasir_set_learn  | TODO |
