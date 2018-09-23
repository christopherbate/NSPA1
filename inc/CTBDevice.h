/*
CTBDevice.h
Christopher Bate
September 2018

This is a further layer on top of SocketDevice (our abstract socket interface)
Basically, it adds the extra layer of reliable transmission that UDP does not provide.
*/
#ifndef CTB_DEVICE_H
#define CTB_DEVICE_H

#include "SocketDevice.h"
#include <vector>
#include <sstream>
#include <deque>
#include <mutex>
#include <queue>

// An arbitrary packet size for our "protocol"
#define PACKET_SIZE 4096

class SendBuffer;
class RecvBuffer;

class CTBDevice
{
  public:
    CTBDevice();
    ~CTBDevice();

    void DestroyDevice();

    /* initialization */
    bool CreateDevice();

    /*
        For reliably sending in-memory data.
    */
    bool SendData(const char *data, uint32_t size);
    uint32_t RecvData(char *data, uint32_t size);

    /*
        Connection
    */
    bool ActiveConnect(string host, string port, uint32_t maxRetry);
    bool Listen(string port, bool &cancel);

    void UpdateSend(bool printDebug = false);
    void UpdateRecv();

    bool Update(bool printDebug = false);

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
        uint32_t dataSize;
    };

    struct Packet
    {
        ProtocolHeader hdr;
        ByteArray data;
    };

    const static uint32_t ACK_MASK = 0x00000001;
    const static uint32_t RST_MASK = 0x00000002;
    const static uint32_t SYN_MASK = 0x00000004;
    const static uint32_t FIN_MASK = 0x00000008;

    /*
        State variables
    */
    SocketDevice m_socket; // Underlying UDP Socket
    bool m_created;
    CtbState m_state;

    SendBuffer *m_sendBuffer;
    RecvBuffer *m_recvBuffer;

    uint64_t m_packetsSent;
    uint64_t m_packetsRecv;
    uint64_t m_ackRecv;
    uint64_t m_ackSent;

    /*
        These are helper functions for resetting manipulating the indices
        and copying the received data round.
    */
    void SendPacket(Packet &packet);
    bool RecvPacket(Packet &pktRecv, uint64_t usecTimeout);

    static void PrintHeader(ProtocolHeader &header)
    {
        cout << "Seq " << std::dec << header.seqNum << endl;
        cout << "Ack " << std::dec << header.ackNum << endl;
        cout << "Flags " << std::hex << header.flags << endl;
        cout << "DataSize " << std::dec << header.dataSize << endl;
    }

    void Print();
};

#endif