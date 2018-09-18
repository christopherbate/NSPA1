#ifndef SEND_BUFFER_H
#define SEND_BUFFER_H
#include "CTBDevice.h"
#include <list>
class SendBuffer
{
  public:
    SendBuffer();
    ~SendBuffer();

    void clear();

    /*
        Each time we receive a packet, call this to update ack's
    */
    void UpdateWithHeader(CTBDevice::ProtocolHeader header);

    /*
        Retrieves the first non-sent Packet        
    */
    CTBDevice::Packet *GetNextAvail();    

    /*  
        After we send a packet, send header here to mark it as sent.
        Force is for retransmission,
        mark this if you receive the same ACK too many times
    */
    void MarkSent(CTBDevice::ProtocolHeader &header, bool force);

    /*
        Removes all items from the buffer that have been sent and ack'd
    */
    void Clean();

    /*
        For retrieving our current window.
    */
    uint32_t GetCapacityRemaining(){
        if( m_maxSize > m_totalSize )
            return m_maxSize - m_totalSize;
        else 
            return 0;
    }

    /*
        Debug
    */
    void Print();

    /*
        Add data to the buffer.
    */
    uint32_t Write(const char *data, uint32_t size);

    /*
        New data added to end of list.
    */
    std::vector<CTBDevice::Packet> m_buffer;

    uint32_t m_lastAckRecv;
    uint32_t m_lastSeqSent;
    uint32_t m_maxSize;
    uint32_t m_currSeqNum;
    uint32_t m_totalSize;

    std::mutex m_lock;
};

#endif