#ifndef RECV_BUFFER_H
#define RECV_BUFFER_H

#include "CTBDevice.h"
#include <list>

class RecvBuffer
{
  public:
    RecvBuffer();
    ~RecvBuffer();

    void clear();

    bool InsertPacket( CTBDevice::Packet *packet );

    uint32_t GetWindow();

    void AdjustExpSeq();

    uint32_t GetNextAck();

    /*
        Reads data, if available.
    */
    uint32_t Read( char *data, uint32_t size );

    /*
        lower numbers are at the front of the list.
    */
    std::list<CTBDevice::Packet*> m_buffer;
    uint32_t m_maxSize;
    uint32_t m_nextExpSeq;
    uint32_t m_totalSize;
    uint32_t m_faultCount;
    std::mutex m_lock;

    void Print();

};

#endif