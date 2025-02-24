# rocksdb/model/Makefile

CXXFLAGS = -std=c++17 -I../include -I.  # Include paths relative to model/

blackbox: common/thread_safe_counter.o blackbox/blackbox.o
	g++ $(CXXFLAGS) common/thread_safe_counter.o blackbox/blackbox.o -o ../blackbox -L../ -L/opt/homebrew/lib -lrocksdb -lz -lbz2 -lzstd -llz4 -pthread

common/thread_safe_counter.o: common/thread_safe_counter.cpp ../common/thread_safe_counter.h
	g++ $(CXXFLAGS) -c common/thread_safe_counter.cpp -o common/thread_safe_counter.o

../common/thread_safe_counter.h: # Explicit rule for the header file (important!)
        # No action needed here, but the rule is required for make to track it

blackbox/blackbox.o: blackbox/blackbox.cpp ../common/thread_safe_counter.h
	g++ $(CXXFLAGS) -c blackbox/blackbox.cpp -o blackbox/blackbox.o

clean:
	rm -rf blackbox/blackbox common/*.o blackbox/*.o