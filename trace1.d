#pragma D option quiet

/* Trace function entry and exit for rocksdb::DB::Get and log probe details */
pid$target:librocksdb_debug.9.11.0.dylib::entry
{
    printf("Probe triggered: provider=%s, module=%s, function=%s, name=%s\n",
           probeprov, probemod, probefunc, probename);
    self->start = timestamp;
}

pid$target:librocksdb_debug.9.11.0.dylib::return
/self->start/
{
    printf("Elapsed time for function %s.%s.%s: %d ns\n", 
           probeprov, probemod, probefunc, timestamp - self->start);
    self->start = 0; /* Reset */
}
