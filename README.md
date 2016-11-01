# expert-octo-tribble

Use a greedy approximation algorithm to find the shortest common superstring of
given strings.

## Build/Runtime Requirements
- Reasonably new compilers for C and C++, e.g. GCC 6 or Clang 3.7. C++14 support
is required.

On Linux also the following libraries are required:
- [libBlocksRuntime](https://github.com/mheily/blocks-runtime)
- [libpthread_workqueue](https://github.com/mheily/libpwq)
- [libkqueue](https://github.com/mheily/libkqueue)

## Build Requirements
- [CMake](http://cmake.org) to build SDSL and libdispatch for Linux.
- [Python2](http://python.org) >= 2.6 to build libdispatch for Linux.


## Building

1. Edit local.mk in the repository root. Useful variables include `CC`, `CXX`,
`LOCAL_CXXFLAGS` and `LOCAL_LDFLAGS`. See common.mk for details.
2. Run make with a suitable numer of parallel jobs, e.g.
`make -j4 BUILD_STYLE=release`


## Running

The tool `tribble/find-superstring/find-superstring` takes a FASTA file as input
and generates an index. On the next run the tool uses the index to generate the
superstring.

See `tribble/find-superstring/find-superstring --help` for command line options.
