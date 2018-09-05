/*
CTBDevice.h
Christopher Bate
September 2018

This is a further layer on top of SocketDevice (our abstract socket interface)
Basically, it adds the extra layer of reliable transmission that UDP doesnt provide.
*/
#ifndef CTB_DEVICE_H
#define CTB_DEVICE_H

#include "SocketDevice.h"
#include <vector>
#include <sstream>

#define CTB_BLOCK_SIZE 16384

class CTBDevice
{
  public:
    CTBDevice();
    ~CTBDevice();

    void DestroyDevice();

    /*for client devices*/
    bool CreateDevice(string host, string port);
    /*for server devices*/
    bool CreateDevice(string port);

    /*
        For reliably sending in-memory data.
    */
    bool SendData(char *data, unsigned long size, unsigned int maxRetry=100);
    bool RecvData(char *data, unsigned long &size);

  private:
    enum CTBType
    {
        CLIENT,
        SERVER
    };
    CTBType m_type;
    SocketDevice m_socket;
    bool m_created;
};

#endif