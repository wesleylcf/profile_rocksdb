#!/bin/bash

SETUP_DB_NAME="setup_db"
POINT_LOOK_NAME="point_lookup"
DTRACE_SCRIPT="probe.d" # Default DTrace script

# Parse arguments
while getopts "p:" opt; do
  case ${opt} in
    p )
      DTRACE_SCRIPT=$OPTARG
      ;;
    \? )
      echo "Usage: $0 [-p dtrace_script]"
      exit 1
      ;;
  esac
done

echo "Compiling $SETUP_DB_NAME...\n"
sudo g++ -std=c++17 -g -O0 -fno-inline profiling/common/setup.cpp -o $SETUP_DB_NAME -I./include -L./ -L/opt/homebrew/lib -lrocksdb -lz -lbz2 -lzstd -llz4 -pthread

echo "Compiling $POINT_LOOK_NAME...\n"
sudo g++ -std=c++17 -g -O0 -fno-inline profiling/point_lookup/get.cpp -o $POINT_LOOK_NAME -I./include -L./ -L/opt/homebrew/lib -lrocksdb -lz -lbz2 -lzstd -llz4 -pthread

echo "Check db empty before writing:\n"
ls /tmp/testdb

./$SETUP_DB_NAME
echo "Executed setup_db, ls /tmp/testdb:\n"
ls /tmp/testdb

echo "Running DTrace with script: $DTRACE_SCRIPT"
sudo dtrace -s "$DTRACE_SCRIPT" -c ./$POINT_LOOK_NAME > probe_output.txt

echo "Executed point_lookup\n"
sudo rm -rf /tmp/testdb/*
