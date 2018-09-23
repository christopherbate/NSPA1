# Network Systems, Programming Assignment 1

# Christopher Bate

# September, 2018

## Introduction

This C++ program implements a stateful reliable transfer protocol over UDP. The overall
mechanism is modeled after TCP, with some simplifications as explained below. A persistent, full-duplex sliding window transfer mechanism with automatic rate control to handle buffer overruns is achieved. Side cases, such as zero-advertised window, retransmission, and connection management are implemented. All requirements in the specification are met, including reliable transfer
and non-blocking operation.

I call the "simplified TCP" protocol that I implement the "CTB" protocol. ("CTB" are
my initials.)

## Compilation

Both a Makefile and Dockerfile are provided for compilation. Simply run "make" in the
program directory. Executables will be generated in the "bin" directory.

## Source Code Organization

All source code C++ files are in the "src" directory, while headers are in the
"inc" directory. The client and server have seperate files for main entry-points,
and they are labeled appropriately. However, Client and Server have significant
code overlap in that they use the same classes as an API into the CTB protocol,
as explained below.

## Architecture

There are several classes in this program.

-   SocketDevice: A simple wrapper around basic UDP operations over sockets
-   CTBDevice: Implements the "CTB Protocol".
-   RecvBuffer: Implements sliding window aspects for the receiving buffer, used by CTBDevice
-   SendBuffer: Similar to the above, but for the sending buffer

The CTBDevice class uses the SocketDevice, RecvBuffer, and SendBuffer classes to implement the CTB protocol.

Non-blocking operation is achieved using threads. Both the client and server have two threads. One thread
runs the CTBProtocol, send and receiving data into the buffers, while the other thread reads and writes data
into the buffers.

## The CTB Protocol

The CTBDevice implements the CTB protocol. Here, I give a basic overview
of the API with an explanation of how each functions works.

-   ActiveConnect: Performs an an "active" connection to the server using 3-way handshake (as described in book)
-   Listen: Listens for connection and performs three way handshake (as descrbed in book)
-   SendData: Writes data given by the user into the SendBuffer
-   RecvData: Takes data (if available) and copies it into a buffer provided by the user
-   Update: If data is in the SendBuffer and the server/client's advertised window is open, this function will send data to the other host. The data that is sent is dictated by sliding window. Which sequence number is next in the queue and which was last acknowledged is handled by the SendBuffer class. Then, it checks for UDP packets, and if they are available, it sends them to RecvBuffer, which handles assembly and local sliding window information.


## Performance 

To test performance of the CTB protocol, I reserved a virtual machine in Taiwan via AWS and sent a variety of file sizes back and forth. Statistical averages and variances of transfer time vs. file size are plotted.