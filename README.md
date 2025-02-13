# Commands
### Build rocksdb
`CXXFLAGS="-arch arm64 -g -O0 -fno-inline" LDFLAGS="-arch arm64 -L/opt/homebrew/lib" DEBUG_LEVEL=2 MAKE_SHARED_LIBS=1 make -j$(sysctl -n hw.logicalcpu)`
* -g(debug mode); -O0(optimization level 0); -fno-inline(disable inlining). These are enabled in case debug function symbols are dropped. Shared library is also enabled

### Compile script to profile
`g++ -std=c++17 -g -O0 -fno-inline <C++ file path> -o <executable name> -I./include -L./ -L/opt/homebrew/lib -lrocksdb -lz -lbz2 -lzstd -llz4 -pthread`

# Scripts
* `probe.d` is to fetch all probes in the form '<provider>:<module>:<function>:<name>' encountered for a given process
* `trace.d` is to get all functions called for a given process
* There's a `run.sh` script which is idempotent and takes a `-p` flag which specifies the path of the `.d` file to use. It will likely be made more configurable in the future.