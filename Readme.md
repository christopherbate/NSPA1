# Network Systems, Programming Assignment 1
# Christopher Bate
# September, 2018

## Introduction 
This C++ program implements a stateful reliable transfer protocol over UDP. The overall
mechanism is modeled after TCP, with some simplifications as explained below. 
All requirements in the specification are met, including reliable transfer 
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
There are two main classes in this program.
- SocketDevice: A simple wrapper around basic UDP operations over sockets
- CTBDevice: Implements the "CTB Protocol". 

The CTBDevice class uses the SocketDevice class to implement the CTB protocol.

## The CTB Protocol 
The CTBDevice implements the CTB protocol. Here, I give a basic overview
of the API with an explanation of how each functions works.