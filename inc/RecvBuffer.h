#ifndef RECV_BUFFER_H
#define RECV_BUFFER_H

#include <list>
#include "CTBDevice.h"

class RecvBuffer
{
  public:
    RecvBuffer();
    ~RecvBuffer();

    void clear();

    void InsertPacket( CTBDevice::Packet *packet );

    uint32_t GetWindow();

    uint32_t GetNextAck();

    /*
        Reads data, if available.
    */
    uint32_t Read( char *data, uint32_t size );

    std::list<CTBDevice::Packet*> m_buffer;
    uint32_t m_maxSize;
    uint32_t m_nextExpSeq;
    uint32_t m_totalSize;
    std::mutex m_lock;
};

#endif