# JamSAT Dependencies

This directory contains git submodules for JamSAT dependencies that
get built along with JamSAT itself.

## JamSAT Shared Object Dependencies
* [`state_ptr`](https://github.com/robbepop/state_ptr) is used to safely
store extra information in pointers.

## JamSAT Frontend Dependencies
* [`CLI11`](https://github.com/CLIUtils/CLI11) is used for option parsing.

## Testing Dependencies
* `minisat` is used to validate JamSAT results in fuzz tests.
