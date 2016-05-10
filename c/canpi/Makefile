CC=g++
LIB= lib/liblog4cpp.a lib/libconfig++.a -Wl,-Bdynamic -lpthread
CFLAGS= -c -Wall -std=c++11 -g -I/usr/local/include -I/usr/include -Ilib/include 
LDFLAGS= -L/usr/local/lib -L/usr/lib -Llib
SOURCES=main.cpp canHandler.cpp tcpServer.cpp tcpClient.cpp 
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=canpi

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIB)

.cpp.o:
	$(CC) $(CFLAGS) $(LIB) $< -o $@

clean:
	rm -f *.o
before: 
	test -d obj || mkdir -p obj
