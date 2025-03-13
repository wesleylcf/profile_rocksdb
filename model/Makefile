# rocksdb/model/Makefile

CXXFLAGS = -std=c++17 -I../include -I. -g # Include paths relative to model/
LDFLAGS = -lrocksdb -luuid -lz -lbz2 -lzstd -llz4 -pthread

generate_data: blackbox/generate_data.o
	g++ $(CXXFLAGS) blackbox/generate_data.o -o ../generate_data $(LDFLAGS)

blackbox/generate_data.o: blackbox/generate_data_seq.cpp
	g++ $(CXXFLAGS) -c blackbox/generate_data_seq.cpp -o blackbox/generate_data.o

clean:
	rm -rf ../generate_data blackbox/*.o