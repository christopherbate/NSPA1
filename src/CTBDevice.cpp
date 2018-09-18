#include "CTBDevice.h"
#include <thread>
/*
constructor
*/
CTBDevice::CTBDevice()
{
    m_created = false;
    m_type = CLIENT;
    m_state = CLOSED;    
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
    m_recvBuffer.clear();
    m_sendBuffer.clear();
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
Transmits a Packet
*/
void CTBDevice::SendPacket(Packet &packet)
{
    // Try to send the packet.
    const uint32_t size = sizeof(ProtocolHeader) + packet.hdr.dataSize;
    char buffer[size];
    memcpy(buffer, &packet.hdr, sizeof(ProtocolHeader));
    copy(packet.data.begin(), packet.data.end(), buffer + sizeof(ProtocolHeader));
    m_socket.BlockingSend(buffer, size);
    m_packetsSent++;
}

/*
Receives a packet, with the given timeout.
*/
bool CTBDevice::RecvPacket(Packet &pktRecv, unsigned int usecTimeout)
{
    m_socket.SetRecvTimeout(usecTimeout);
    string recv_addr;
    char buffer[PACKET_SIZE];
    uint32_t size = m_socket.BlockingRecv(buffer, PACKET_SIZE, recv_addr, NULL);
    memcpy(&pktRecv.hdr, buffer, sizeof(ProtocolHeader));
    if (size == 0)
    {
        return false;
    }
    //PrintHeader(pktRecv.hdr);
    if (pktRecv.hdr.dataSize > 0 && pktRecv.hdr.dataSize + sizeof(ProtocolHeader) < PACKET_SIZE)
    {
        if (size - sizeof(ProtocolHeader) != pktRecv.hdr.dataSize)
        {
            cerr << "RecvPacket : Data not accumulated correctly, " << size << "/" << pktRecv.hdr.dataSize << " dropping packet" << endl;
            return false;
        }
        pktRecv.data.clear();
        pktRecv.data.insert(pktRecv.data.end(), &buffer[sizeof(ProtocolHeader)], &buffer[pktRecv.hdr.dataSize + sizeof(ProtocolHeader)]);
    }
    m_packetsRecv++;
    return true;
}

/*
ActiveConnect
blocking
*/
bool CTBDevice::ActiveConnect(string host, string port, uint32_t maxRetry)
{
    m_state = CLOSED;

    // Reset all buffer indices
    m_sendBuffer.clear();
    m_recvBuffer.clear();

    // Wait for Ack. Allow for 1 sec before resending
    uint32_t retryNum = 0;

    Packet pkt;
    pkt.hdr.flags = SYN_MASK;
    pkt.hdr.seqNum = 0;
    pkt.hdr.dataSize = 0;

    while (retryNum < maxRetry)
    {
        SendPacket(pkt);

        Packet recv;

        if (RecvPacket(recv, 100))
        {
            cout << "ActiveConnect: Received response" << endl;
            // Must have ack and syn flags sent
            if ((recv.hdr.flags & (ACK_MASK | SYN_MASK)) == (ACK_MASK | SYN_MASK) && recv.hdr.ackNum == 1)
            {
                cout << "ActiveConnect: Connection established" << endl;
                m_state = ESTABLISHED;
                break;
            }
        }
        std::this_thread::sleep_for(chrono::milliseconds(1000));
        retryNum++;
    }

    // Debug
    if (m_state == ESTABLISHED)
    {
        // Transmit (potentially with loss, but can recover (see book TCP description))
        pkt.hdr.seqNum = 0;
        pkt.hdr.advWindow = m_maxRecvBuffer;
        pkt.hdr.ackNum = 1;
        pkt.hdr.flags = ACK_MASK;
        pkt.hdr.dataSize = 0;
        //cout << "ActiveConnect : Sending 3rd Msg" << endl;
        SendPacket(pkt);
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
    m_sendBuffer.clear();
    m_recvBuffer.clear();
    m_state = LISTEN;

    // Wait for incoming data.
    Packet pkt;

    while (!cancel)
    {
        if (RecvPacket(pkt, 1000000))
        {
            if (pkt.hdr.flags & SYN_MASK)
            {
                //cout << "Listen : received request SYN" << endl;
                m_state = SYN_RECV;
                break;
            }
        }
    }

    if (m_state == SYN_RECV)
    {
        // Send ack and wait for response.
        // Wait for Ack. Allow for 1 sec before resending
        // Create header with initial seq num
        Packet ack;
        ack.hdr.seqNum = 0;
        ack.hdr.ackNum = 1;
        ack.hdr.dataSize = 0;
        ack.hdr.advWindow = m_recvBuffer.GetWindow();
        ack.hdr.flags = SYN_MASK | ACK_MASK;
        uint32_t retryNum = 0;
        uint32_t maxRetry = 120;
        //cout << "Listen : Sending 2nd msg" << endl;
        SendPacket(ack);       
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

/*
Update
Updates the buffers as necessary
*/
bool CTBDevice::Update()
{

}

/*
SendData
Adds a new packet to the queue
*/
bool CTBDevice::SendData(const char *data, uint32_t size)
{
  
}

/*
RecvData
retrieves a the next packet.
*/
uint32_t CTBDevice::RecvData(char *data, uint32_t size)
{
}