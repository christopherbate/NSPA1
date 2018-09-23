CC=g++
CFLAGS=-I$(IDIR) -Wall -Werror -std=c++11
IDIR=./inc
ODIR=obj
LIBS=-pthread

_DEPS = SocketDevice.h CTBDevice.h SendBuffer.h RecvBuffer.h Shared.h BasicTests.h
DEPS = $(pathsubst %,$(IDIR)/%,$(_DEPS))

all: bin/tests bin/client bin/server

$(ODIR)/%.o: src/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

bin/tests: obj/test_runner.o obj/RecvBuffer.o obj/SendBuffer.o obj/SocketDevice.o obj/CTBDevice.o obj/BasicTests.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

bin/client: obj/client_main.o obj/RecvBuffer.o obj/SendBuffer.o obj/SocketDevice.o obj/CTBDevice.o 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

bin/server: obj/server_main.o obj/RecvBuffer.o obj/SendBuffer.o obj/SocketDevice.o obj/CTBDevice.o 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o bin/*


