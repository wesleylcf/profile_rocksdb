# Commands
### Build rocksdb
`CXXFLAGS="-arch arm64 -g -O0" LDFLAGS="-arch arm64 -L/opt/homebrew/lib" DEBUG_LEVEL=2 MAKE_SHARED_LIBS=1 make -j$(sysctl -n hw.logicalcpu)`
* This builds in debug mode, with no optimization(in case debug function symbols are dropped), and enables shared library.

### Compile script to profile
`g++ -std=c++17 -g -O0 profiling/point_lookup/dtrace.cpp -o point_lookup_test -I./include -L./ -L/opt/homebrew/lib -lrocksdb -lz -lbz2 -lzstd -llz4 -pthread`

# Scripts
* `probe.d` is to fetch all probes in the form '<provider>:<module>:<function>:<name>' encountered for a given process
* `trace.d` is to get all functions called for a given process
* `dtrace.cpp` is the script used to instantiate a database instance, write and read to it.
* `sort_trace_output.cpp` is the script meant to be run after `dtrace.cpp` to sort the output in descending time elapsed.