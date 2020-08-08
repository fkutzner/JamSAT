# JamSAT Dependencies

This directory contains git submodules for JamSAT dependencies that
get built along with JamSAT itself.

## JamSAT Shared Object Dependencies
* [`state_ptr`](https://github.com/robbepop/state_ptr) is used to safely
store extra information in pointers. The definitions of `state_ptr` are hidden
in a JamSAT-specific namespace.
* [`GSL`](https://github.com/microsoft/GSL), the Microsoft implementation of
the Guideline Support Library. See the CMake option
`JAMSAT_USE_SYSTEM_GSL` if you wish to build JamSAT with a different version
of `GSL`.

## JamSAT Frontend Dependencies
* [`CLI11`](https://github.com/CLIUtils/CLI11) is used for option parsing.

## Testing Dependencies
* `minisat` is used to validate JamSAT results in fuzz tests.
