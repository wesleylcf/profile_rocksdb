#!/bin/bash

cd ../.. # move the root repo directory

# Find the librocksdb.so files
LIB_FILES=$(find . -name "librocksdb*.so*")

if [ -z "$LIB_FILES" ]; then
  echo "Error: No librocksdb files found."
  exit 1
fi

# Copy the librocksdb files to /usr/local/lib
sudo cp $LIB_FILES /usr/local/lib/

#Create the symbolic links.
cd /usr/local/lib
sudo ln -sf librocksdb.so.9.11.0 librocksdb.so.9.11
sudo ln -sf librocksdb.so.9.11 librocksdb.so.9
sudo ln -sf librocksdb.so.9 librocksdb.so

# Update the shared library cache
sudo ldconfig