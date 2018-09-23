CC=g++
CFLAGS=-I$(IDIR) -Wall -Werror -std=c++11
IDIR=./inc
ODIR=obj
LIBS=-pthread

_DEPS = SocketDevice.h CTBDevice.h SendBuffer.h RecvBuffer.h Shared.h BasicTests.h Request.h Response.h
DEPS = $(pathsubst %,$(IDIR)/%,$(_DEPS))

all: bin/tests client_bin/client server_bin/server

$(ODIR)/%.o: src/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client_bin/client: obj/client_main.o obj/RecvBuffer.o obj/SendBuffer.o obj/SocketDevice.o obj/CTBDevice.o obj/Request.o obj/Response.o obj/Shared.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server_bin/server: obj/server_main.o obj/RecvBuffer.o obj/SendBuffer.o obj/SocketDevice.o obj/CTBDevice.o obj/Request.o obj/Response.o obj/Shared.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

bin/tests: obj/test_runner.o obj/RecvBuffer.o obj/SendBuffer.o obj/SocketDevice.o obj/CTBDevice.o obj/Request.o obj/Response.o obj/Shared.o obj/BasicTests.o 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)



.PHONY: clean

clean:
	rm -f $(ODIR)/*.o bin/tests server_bin/server client_bin/client


