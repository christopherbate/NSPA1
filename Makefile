CC=g++
CFLAGS=-I$(IDIR)
IDIR=./inc
ODIR=obj
LIBS=-pthread

_DEPS = SocketDevice.h CTBDevice.h
DEPS = $(pathsubst %,$(IDIR)/%,$(_DEPS))

all: bin/tests bin/client bin/server

$(ODIR)/%.o: src/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

bin/tests: obj/test_runner.o obj/SocketDevice.o obj/CTBDevice.o obj/RecvBuffer.o obj/SendBuffer.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

bin/client: obj/client_main.o obj/SocketDevice.o obj/CTBDevice.o obj/RecvBuffer.o obj/SendBuffer.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

bin/server: obj/server_main.o obj/SocketDevice.o obj/CTBDevice.o obj/RecvBuffer.o obj/SendBuffer.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o bin/*


