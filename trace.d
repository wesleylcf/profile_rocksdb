#!/usr/sbin/dtrace -s

// Define the probes for entry and exit of functions related to 'rocksdb::'
pid$target:::entry {
    self->start_time = timestamp; // Capture the start time of the function call
}

pid$target:::return {
    // Calculate the time elapsed by subtracting start_time from the current timestamp
    printf("%s took %llu ns", probefunc, timestamp - self->start_time);
    self->start_time = 0; // Reset the start time
}
