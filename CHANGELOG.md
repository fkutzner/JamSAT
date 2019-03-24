# JamSAT Changelog

This changelog's format is based on [keep a changelog 1.0.0](https://keepachangelog.com/en/1.0.0/).

## [0.2.0] - 2019-03-24
### Added
- Cheap subsumption and self-subsuming resolution optimizations to eliminate
  variables occurring in unary clauses during search
- Failed literal elimination
- Subsumption and self-subsuming resolution optimizations using a variant of
  hyper-binary resolution
- Extended documentation: added repository description and instructions about
  how to include JamSAT in client projects
- Optimization: recomputing the LBD values of reason clauses on the current
  decision level during backtracking

### Changed
- (internal) Simplified the build process by automatically building the Minisat
  test dependency
- (internal) Overhauled the build system
- (internal) Overhauled named requirements by introducing concepts-like type traits
  for JamSAT concepts
- (internal) Simplified the CDCL driver and the simplifier implementations

## [0.1.0] - 2018-05-21
### Added
- Basic CDCL solver
- IPASIR API implementation
- Standalone frontend
