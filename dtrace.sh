#!/bin/bash

# Usage message
usage() {
  echo "Usage: sudo sh dtrace.sh -t <dtrace file path> -e <executable file path>"
  exit 1
}

SETUP_DB_NAME="setup_db"
EXEC_NAME="point_lookup"
DTRACE_SCRIPT="probe.d" # Default DTrace script

# Parse arguments
while getopts "t:e:" opt; do
  case ${opt} in
    t )
      DTRACE_SCRIPT=$OPTARG
      ;;
    e )
      EXEC_NAME=$OPTARG
      ;;
    \? )
      usage
      ;;
  esac
done

echo "Compiling $SETUP_DB_NAME...\n"
sudo g++ -std=c++17 -g -O0 -fno-inline profiling/common/setup.cpp -o $SETUP_DB_NAME -I./include -L./ -L/opt/homebrew/lib -lrocksdb -lz -lbz2 -lzstd -llz4 -pthread

echo "Compiling $EXEC_NAME...\n"
sudo g++ -std=c++17 -g -O0 -fno-inline profiling/point_lookup/get.cpp -o $EXEC_NAME -I./include -L./ -L/opt/homebrew/lib -lrocksdb -lz -lbz2 -lzstd -llz4 -pthread

echo "Check db empty before writing:\n"
ls /tmp/testdb

./$SETUP_DB_NAME
echo "Executed setup_db, ls /tmp/testdb:\n"
ls /tmp/testdb

echo "Running DTrace with script: $DTRACE_SCRIPT"

# Ensure profiling directory exists
mkdir -p profiling/$EXEC_NAME

sudo dtrace -s $DTRACE_SCRIPT -c ./$EXEC_NAME > "profiling/$EXEC_NAME/dtrace.txt"

echo "Executed point_lookup\n"
sudo rm -rf /tmp/testdb/*
