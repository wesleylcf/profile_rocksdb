# rocksdb/model/Makefile

CXXFLAGS = -std=c++17 -I../include -I. -g # Include paths relative to model/

generate_data: common/thread_safe_counter.o blackbox/generate_data.o
	g++ $(CXXFLAGS) common/thread_safe_counter.o blackbox/generate_data.o -o ../generate_data -L../ -L/opt/homebrew/lib -lrocksdb -lz -lbz2 -lzstd -llz4 -pthread

common/thread_safe_counter.o: common/thread_safe_counter.cpp ../common/thread_safe_counter.h
	g++ $(CXXFLAGS) -c common/thread_safe_counter.cpp -o common/thread_safe_counter.o

../common/thread_safe_counter.h: # Explicit rule for the header file (important!)
        # No action needed here, but the rule is required for make to track it

# blackbox/generate_data.o: blackbox/blackbox.cpp ../common/thread_safe_counter.h
# 	g++ $(CXXFLAGS) -c blackbox/generate_data_concurrent.cpp -o blackbox/generate_data.o
blackbox/generate_data.o: blackbox/generate_data_seq.cpp ../common/thread_safe_counter.h
	g++ $(CXXFLAGS) -c blackbox/generate_data_seq.cpp -o blackbox/generate_data.o

clean:
	rm -rf blackbox/generate_data common/*.o blackbox/*.o