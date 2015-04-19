all:
	g++ -g -pthread -o ospf_local main.cpp lsa.cpp hello.cpp ospf.cpp
