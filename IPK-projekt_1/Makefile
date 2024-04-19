CXX := g++
CXXFLAGS := -std=c++11 -Wall -pthread -finput-charset=UTF-8

all: ipk24chat-client

ipk24chat-client: main.o tcp.o udp.o
	$(CXX) $(CXXFLAGS) -o $@ $^

main.o: main.cpp tcp.hpp udp.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

tcp.o: tcp.cpp tcp.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

udp.o: udp.cpp udp.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o ipk24chat-client
