#!/bin/bash

# Clean and build RocksDB
make clean
make -j$(($(nproc) - 1))

cd ./model/common
sh move_shared_libs_win.sh

# Navigate back to the model directory and build the application
cd model
make clean
make generate_data

# Run the application
cd ..
./generate_data

echo "RocksDB build and installation complete."