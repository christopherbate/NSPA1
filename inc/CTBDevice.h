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
#include "SegmentTracker.h"
#include <vector>
#include <sstream>
#include <deque>
#include <queue>

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
    bool SendData(char *data, uint32_t size);
    bool RecvData(char *data, uint32_t &size);

    /*
        Connection
    */
    bool ActiveConnect(string host, string port, uint32_t maxRetry);
    bool Listen(string port,bool &cancel);
    bool UpdateSend();

    // States  of the protocol, straight out of the TCP diagram from the book.
    enum CtbState
    {
        CLOSED,
        LISTEN,
        SYN_RECV,
        SYN_SENT,
        ESTABLISHED,
        FIN_WAIT_1,
        FIN_WAIT_2,
        CLOSING,
        TIME_WAIT,
        CLOSE_WAIT,
        LAST_ACK
    };

    /*
        This is our flexible datatype for holding 
        dynamic arrays in the send/recv queue.
    */
    typedef std::vector<char> ByteArray;

    /*
        ProtocolHeader
        Differs from TCP header
        I used 6x32 bit ints in this struct in an attempt
        to avoid packing issues that might result from different compilers, etc.
    */
    struct ProtocolHeader
    {
        uint32_t seqNum;
        uint32_t ackNum;
        /*
            Flags, starting from LSB, are:
            ACK, RST, SYN, FIN
        */
        uint32_t flags;
        uint32_t advWindow;
    };

    const uint32_t ACK_MASK = 0x00000001;
    const uint32_t RST_MASK = 0x00000002;
    const uint32_t SYN_MASK = 0x00000004;
    const uint32_t FIN_MASK = 0x00000008;

  private:
    enum CTBType
    {
        CLIENT,
        SERVER
    };
    CTBType m_type;
    SocketDevice m_socket; // Underlying UDP Socket
    bool m_created;
    CtbState m_state;

    /*
        These are the queues for recv/sending data.
    */
    ByteArray m_recvBuffer;
    SegmentTracker m_segTracker;
    ByteArray m_sendBuffer;

    /*        
        These are indicies into the current ByteArray for 
        sliding window - SEND BUFFER
    */
    uint32_t m_lastByteAcked;
    uint32_t m_lastByteSent;
    uint32_t m_lastByteWritten;
    uint32_t m_effWindow;

    /*
        These are indices into the current ByteArray
        for sliding window - RECV BUFFER
    */
    uint32_t m_lastByteRead;
    uint32_t m_nextByteExpected;
    uint32_t m_lastByteRecv;
    uint32_t m_maxRecvBuffer;
    uint32_t m_advWindow;

    /* 
        This is a very special number - 
        what sequence number idx 0 in the recv buffer corresponds to.
        Referred to as "s" in the documenation I provide.
    */
    uint32_t m_recvIdxZeroSeqNum;
    uint32_t m_sendIdxZeroSeqNum;

    /*
        These are helper functions for resetting manipulating the indices
        and copying the received data round.
    */
    void ResetBufferIndices();
    void ProcessRecvHeader(ProtocolHeader &header, uint32_t dataSize);
    void ProcessRecvPayload(ProtocolHeader &header, char *data, uint32_t length);

    /*
        Debug functions.
    */
    void PrintRecvBuffer()
    {
        if (!m_recvBuffer.empty())
        {
            std::cout << string(&m_recvBuffer[0], m_recvBuffer.size()) << std::endl;
        }
        else
        {
            std::cout << "Emtpy Recv Buffer" << std::endl;
        }
    }
};

#endif