#include "CTBDevice.h"

/*
constructor
*/
CTBDevice::CTBDevice()
{
    m_created = false;
    m_type = CLIENT;
    m_state = CLOSED;
    m_maxRecvBuffer = 10;
}

/*
Destructor
*/
CTBDevice::~CTBDevice()
{
    // Always safe.
    m_socket.CloseSocket();
}

/*
DestroyDevice
*/
void CTBDevice::DestroyDevice()
{
    m_created = false;
    m_state = CLOSED;
    m_socket.CloseSocket();
}

/*
CreateDevice (client mode)
*/
bool CTBDevice::CreateDevice(string host, string port)
{
    if (m_created)
        return false;

    if (!m_socket.CreateSocket(host, port))
    {
        return false;
    }

    m_created = true;
    m_type = CLIENT;
    m_state = CLOSED;
    return true;
}

void CTBDevice::ResetBufferIndices()
{
    m_lastByteSent = 0;
    m_lastByteAcked = 0;
    m_lastByteWritten = 0;
    m_effWindow = 0;
    m_lastByteRead = 0;
    m_nextByteExpected = 0;
    m_lastByteRecv = 0;
    m_maxRecvBuffer = 0;
    m_advWindow = 0;
}

/*
    ProcessRecvHeader
    Updates the indices in the received buffer and acked position in the 
    sending buffer
    // ROLLOVER
*/
void CTBDevice::ProcessRecvHeader(ProtocolHeader &header, uint32_t dataSize)
{
    // Always track acknowledgements.
    if (header.flags & ACK_MASK)
    {
        // On acknowledge - store commulative ACK position, and compute window
        m_lastByteAcked = header.ackNum - 1;
        m_effWindow = header.advWindow - (m_lastByteSent - m_lastByteAcked);
    }

    // Seq case 1 - connection setup
    if (header.flags & SYN_MASK)
    {
        // On Connection - Establish senders
        // sequence starting number
        m_nextByteExpected = header.seqNum + 1;
        m_lastByteRecv = header.seqNum + dataSize - 1;
        m_recvIdxZeroSeqNum = m_nextByteExpected;
    }
    // Seq case 2 - data received.
    else
    {
        // Add all bytes received... probably should make
        // this more efficient O(1), but writing the segTracker was annoying enough.
        for (uint32_t i = header.seqNum; i <= m_lastByteRecv; i++)
            m_segTracker.AddByte(i);
        m_nextByteExpected = m_segTracker.GetFirstGapIndex();
    }
}

/*
    ProcessRecvPayload
    This function copies the received payload from a CTB Packet into the 
    dynamic receiving buffer in the correct place.
    Note that header is not coppied.
*/
void CTBDevice::ProcessRecvPayload(ProtocolHeader &header, char *data, uint32_t length)
{
    // Subtract the header from the amount of data we need to copy.
    if (length < sizeof(header))
    {
        std::cerr << "Warning: Packet received smaller than header." << std::endl;
        return;
    }
    length = length - sizeof(header);
    if (length == 0)
        return;

    // Calculate indices in the buffer.
    uint32_t startIndex = header.seqNum - m_recvIdxZeroSeqNum;
    uint32_t endIndex = header.seqNum + length - 1 - m_recvIdxZeroSeqNum;

    // pre-extend the vector if necessary.
    // No cost if the buffer is already big enough
    m_recvBuffer.reserve(endIndex + 1);

    // Copy in the data, without the header.
    memcpy(&m_recvBuffer[startIndex], data + sizeof(header), length);
}

/*
ActiveConnect
blocking
*/
bool CTBDevice::ActiveConnect(string host, string port, uint32_t maxRetry)
{
    m_state = CLOSED;
    m_segTracker.ClearSegments();

    // Create header with initial seq num and window size.
    ProtocolHeader hdr;
    hdr.seqNum = 0;
    m_sendIdxZeroSeqNum = 1;
    hdr.flags = SYN_MASK;

    // Reset all buffer indices
    ResetBufferIndices();

    // Wait for Ack. Allow for 1 sec before resending
    char buffer[PACKET_SIZE];
    m_socket.SetRecvTimeout(1000000);
    uint32_t retryNum = 0;
    string recv_addr;

    while (retryNum < maxRetry)
    {
        cout << "ActiveConnect : Sending header with flags " << std::hex << hdr.flags << endl;
        m_socket.BlockingSend((char *)&hdr, sizeof(ProtocolHeader));
        m_state = SYN_SENT;
        uint32_t size = m_socket.BlockingRecv(buffer, PACKET_SIZE, recv_addr, NULL);
        if (!m_socket.DidTimeout() && size > 0)
        {
            cout << "ActiveConnect: Received response" << endl;
            // Get header.
            ProtocolHeader respHdr;
            memcpy(&respHdr, buffer, sizeof(ProtocolHeader));
            // Must have ack and syn flags sent
            if ((respHdr.flags & (ACK_MASK | SYN_MASK)) == (ACK_MASK | SYN_MASK) && respHdr.ackNum == 1)
            {
                cout << "ActiveConnect: Connection established" << endl;
                m_state = ESTABLISHED;
                ProcessRecvHeader(respHdr, size);
                break;
            }
        }
        retryNum++;
    }

    // Debug
    if (m_state == ESTABLISHED)
    {
        cout << "ActiveConnect : Peer starting sequence num " << hdr.seqNum << endl;

        // Transmit (potentially with loss, but can recover (see book TCP description))
        hdr.seqNum = 0;
        m_sendIdxZeroSeqNum = 1;
        hdr.advWindow = 512;
        hdr.ackNum = m_nextByteExpected;
        hdr.flags = ACK_MASK;
        m_socket.BlockingSend((char *)&hdr, sizeof(ProtocolHeader));
    }
    else
    {
        cout << "ActiveConnect: Retry count exceeded." << endl;
        m_state = CLOSED;
        return false;
    }

    return true;
}

/*
Listen
blocking
*/
bool CTBDevice::Listen(string port, bool &cancel)
{
    m_state = LISTEN;
    m_segTracker.ClearSegments();

    // Wait for incoming data.
    ProtocolHeader hdr;
    uint32_t size = 0;
    char buffer[PACKET_SIZE];
    m_socket.SetRecvTimeout(0);
    string recv_addr;
    while (!cancel)
    {
        size = m_socket.BlockingRecv(buffer, PACKET_SIZE, recv_addr, NULL);
        if (!m_socket.DidTimeout() && size > 0)
        {
            // Get header.
            memcpy(&hdr, buffer, sizeof(hdr));
            // Must have ack and syn flags sent
            if (hdr.flags & (SYN_MASK))
            {
                cout << "Listen : received request SYN" << endl;
                m_state = SYN_RECV;
                ProcessRecvHeader(hdr, size);
                break;
            }
            else
            {
                cout << "Listen : received request no SYN mask " << std::hex << hdr.flags << endl;
            }
        }
    }

    if (m_state == SYN_RECV)
    {
        // Send ack and wait for response.
        // Wait for Ack. Allow for 1 sec before resending
        // Create header with initial seq num
        m_lastByteRecv = hdr.seqNum;
        hdr.seqNum = 0;
        m_sendIdxZeroSeqNum = 1;
        hdr.ackNum = m_lastByteRecv + 1;
        hdr.advWindow = m_maxRecvBuffer;
        hdr.flags = SYN_MASK | ACK_MASK;
        uint32_t retryNum = 0;
        uint32_t maxRetry = 120;

        while (retryNum < maxRetry)
        {
            m_socket.SetRecvTimeout(1000000);
            m_socket.BlockingSend((char *)&hdr, sizeof(hdr));
            size = m_socket.BlockingRecv(buffer, PACKET_SIZE, recv_addr, NULL);
            if (!m_socket.DidTimeout())
            {
                ProtocolHeader respHdr;
                // Get header.
                memcpy(&respHdr, buffer, sizeof(ProtocolHeader));
                // Must have ack sent
                if ((respHdr.flags & (ACK_MASK)) && respHdr.ackNum == 1)
                {
                    ProcessRecvHeader(respHdr, size);
                    m_state = ESTABLISHED;
                    cout << "Listen : Connection established" << endl;
                    return true;
                }
            }
            retryNum++;
        }

        cout << "Listen : Retry count exceeded, failed after SYN_RECV" << endl;
        return false;
    }
    else
    {
        if (cancel)
        {
            cout << "Listen : canceling." << endl;
        }
        return false;
    }

    return true;
}

bool CTBDevice::UpdateSend()
{
    if (m_state == ESTABLISHED)
    {
        // If I have data, send it.
        if (m_sendBuffer.size() > 0)
        {
            ProtocolHeader header;
            header.seqNum = (m_lastByteAcked + 1);
            header.flags = ACK_MASK;
            header.ackNum = m_nextByteExpected;
            header.advWindow = m_maxRecvBuffer;

            // Send as much data as possible.
            uint32_t hdrSize = sizeof(header);
            uint32_t dataSize = min<uint32_t>(PACKET_SIZE - sizeof(header), m_effWindow);

            // Check if the limit is zero.
            if (dataSize == 0)
            {
                return false;
            }

            // Send the maximum amount of data.
            if (m_sendBuffer.size() < dataSize)
                dataSize = m_sendBuffer.size();

            std::cout << "Sending " << hdrSize << " header " << dataSize << " data " << std::endl;

            // Copy the header
            char buffer[PACKET_SIZE];
            memcpy(buffer, &header, hdrSize);
            memcpy(buffer + hdrSize, &m_sendBuffer[0], dataSize);
            m_socket.BlockingSend(&m_sendBuffer[0], hdrSize + dataSize, NULL);
        }
    }
}

/*
CreateDevice (server mode)
*/
bool CTBDevice::CreateDevice(string port)
{
    if (m_created)
        return false;
    if (!m_socket.CreateSocket(port))
        return false;

    m_created = true;
    m_type = SERVER;
    return true;
}

/*
SendData
For reliably sending in-memory data.
*/
bool CTBDevice::SendData(char *data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {

    }
    return true;
}

/*
RecvData
*/
bool CTBDevice::RecvData(char *data, uint32_t &size)
{

    return true;
}