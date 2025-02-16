#pragma D option quiet

pid$target:librocksdb_debug.9.11.0.dylib::entry {
    printf("ENTRY|%s\n", probefunc);
    self->start_time = timestamp; // Capture the start time of the function call
}

pid$target:librocksdb_debug.9.11.0.dylib::return {
    // Calculate the time elapsed by subtracting start_time from the current timestamp
    this->elapsed_time = timestamp - self->start_time;
    printf("RETURN|%s|%lu\n", probefunc, this->elapsed_time); // Use | as a separator
    self->start_time = 0; // Reset the start time
}