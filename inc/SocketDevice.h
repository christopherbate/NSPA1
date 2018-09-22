/*
SocketDevice.h
Christopher Bate
September 2018

This is meant to be a layer to abstract away the actual socket calls. In any cross-platform extentions, this would be an interface 
that would be implemented by different platform-specific classes (Unix, Windows, etc).
For this project, we don't bother with an interface and instead just add some basic unix socket functionalit.
*/
#ifndef SOCKET_DEVICE_H
#define SOCKET_DEVICE_H

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

using namespace std;

class SocketDevice
{
  public:
    SocketDevice();
    ~SocketDevice();

    int GetFd() { return m_fd; }

    void CloseSocket();

    bool SetRecvTimeout( uint64_t usec);
    bool DidTimeout() { return m_timeout; }

    bool CreateSocket(string host, string port);
    bool CreateSocket(string port);

    uint32_t BlockingSend(const char *data, unsigned long length, struct sockaddr *remoteAddr = NULL);

    uint32_t BlockingRecv(char *buffer, unsigned long size, string &recvAddress, struct sockaddr *remoteAddr = NULL);

  private:
    enum SDType
    {
        CONN,
        PASSIVE
    };
    SDType m_type;
    int m_fd;
    string m_remoteHost;
    bool m_timeout;
    string m_remotePort;
    struct sockaddr_storage m_remoteAddr;
};

#endif