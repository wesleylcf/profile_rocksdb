#pragma D option quiet

pid$target::rocksdb::GetImpl:entry
{
    printf("Entering RocksDB GetImpl(): key = %s\n", copyinstr(arg0));
    self->start = timestamp;
}

pid$target::rocksdb::GetImpl:return
/self->start/
{
    printf("Exiting RocksDB GetImpl(): Elapsed time = %d ns\n", timestamp - self->start);
    self->start = 0; /* Reset */
}
