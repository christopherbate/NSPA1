#include "CTBDevice.h"
#include <thread>
#include "SendBuffer.h"
#include "RecvBuffer.h"
/*
constructor
*/
CTBDevice::CTBDevice()
{
    m_created = false;
    m_state = CLOSED;
    m_sendBuffer = NULL;
    m_recvBuffer = NULL;
    m_packetsRecv = m_packetsSent = m_ackSent = m_ackRecv = 0;
}

/*
Destructor
*/
CTBDevice::~CTBDevice()
{
    // Always safe.
    m_socket.CloseSocket();

    if (m_created)
    {
        if (m_sendBuffer != NULL)
        {
            delete m_sendBuffer;
            m_sendBuffer = NULL;
        }
        if (m_recvBuffer != NULL)
        {
            delete m_recvBuffer;
            m_recvBuffer = NULL;
        }
    }
}

/*
DestroyDevice
*/
void CTBDevice::DestroyDevice()
{
    if (m_created)
    {
        m_recvBuffer->clear();
        m_sendBuffer->clear();
        if (m_sendBuffer != NULL)
        {
            delete m_sendBuffer;
            m_sendBuffer = NULL;
        }
        if (m_recvBuffer != NULL)
        {
            delete m_recvBuffer;
            m_recvBuffer = NULL;
        }
    }
    m_created = false;
    m_state = CLOSED;
    m_socket.CloseSocket();
}
/*
CreateDevice (client mode)
*/
bool CTBDevice::CreateDevice()
{
    if (m_created)
    {
        return false;
    }

    m_sendBuffer = new SendBuffer();
    m_recvBuffer = new RecvBuffer();

    m_created = true;
    m_state = CLOSED;
    return true;
}

/*
ActiveConnect
blocking
*/
bool CTBDevice::ActiveConnect(string host, string port, uint32_t maxRetry)
{
    if (!m_created)
        return false;
    m_socket.CloseSocket();
    if (!m_socket.CreateSocket(host, port))
    {
        return false;
    }

    m_state = CLOSED;

    // Reset all buffer indices
    m_sendBuffer->clear();
    m_recvBuffer->clear();

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

        if (RecvPacket(recv, 1000000))
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
        retryNum++;
    }

    // Debug
    if (m_state == ESTABLISHED)
    {
        // Transmit (potentially with loss, but can recover (see book TCP description))
        pkt.hdr.seqNum = 0;
        pkt.hdr.advWindow = m_recvBuffer->GetWindow();
        pkt.hdr.ackNum = 1;
        pkt.hdr.flags = ACK_MASK;
        pkt.hdr.dataSize = 0;
        //cout << "ActiveConnect : Sending 3rd Msg" << endl;
        SendPacket(pkt);
    }
    else
    {
        //cout << "ActiveConnect: Retry count exceeded." << endl;
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
    if (!m_created)
        return false;

    if (!m_socket.CreateSocket(port))
    {
        return false;
    }

    m_sendBuffer->clear();
    m_recvBuffer->clear();
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
        while (!cancel)
        {
            // Send ack and wait for response.
            // Wait for Ack. Allow for 1 sec before resending
            // Create header with initial seq num
            Packet ack;
            ack.hdr.seqNum = 0;
            ack.hdr.ackNum = 1;
            ack.hdr.dataSize = 0;
            ack.hdr.advWindow = m_recvBuffer->GetWindow();
            ack.hdr.flags = SYN_MASK | ACK_MASK;
            //cout << "Listen : Sending 2nd msg" << endl;
            SendPacket(ack);

            if (RecvPacket(pkt, 1000000))
            {
                if (pkt.hdr.flags & ACK_MASK)
                {
                    m_state = ESTABLISHED;
                    cout << "Server established connection." << endl;
                    break;
                }
            }
        }
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
}

/*
Receives a packet, with the given timeout.
*/
bool CTBDevice::RecvPacket(Packet &pktRecv, uint64_t usecTimeout)
{
    m_socket.SetRecvTimeout(usecTimeout);
    string recv_addr;
    char buffer[2 * PACKET_SIZE];
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
    return true;
}

void CTBDevice::UpdateSend(bool printDebug)
{
    static auto zwTimer = chrono::steady_clock::now(); // zero window timer
    static auto lpTimer = chrono::steady_clock::now(); // Last packet timer
    static uint32_t lastAck = 1;

    CTBDevice::Packet *pkt;
    do
    {
        // Check if we've exceeded effective window.
        // If so, start a timer.
        if (m_sendBuffer->GetInFlightBytes() >= m_sendBuffer->m_effWindow)
        {
            uint32_t elapsed = chrono::duration_cast<chrono::milliseconds>(
                                   chrono::steady_clock::now() - zwTimer)
                                   .count();
            if (elapsed > 100)
            {
                if(printDebug)
                    cout << "Sender has zero window, sending timeout inquiry" << endl;
                zwTimer = chrono::steady_clock::now();
            }
            else
            {
                break;
            }
        }

        // Get the next available packet and send if available.
        pkt = m_sendBuffer->GetNextAvail();
        if (pkt != NULL)
        {
            //cout << "Sending" << endl;
            pkt->hdr.flags = ACK_MASK;
            pkt->hdr.ackNum = m_recvBuffer->GetNextAck();
            pkt->hdr.advWindow = m_recvBuffer->GetWindow();
            lastAck = pkt->hdr.ackNum;
            SendPacket(*pkt);
            m_sendBuffer->MarkSent(pkt->hdr);
            m_packetsSent++;
        }
        else
        {
            // Send an ack
            uint32_t elapsed = chrono::duration_cast<chrono::milliseconds>(
                                   chrono::steady_clock::now() - lpTimer)
                                   .count();
            if (m_recvBuffer->GetNextAck() != lastAck || elapsed > 30)
            {
                //cout << "Sender sending keep-alive signal." << endl;
                Packet ack;
                ack.hdr.ackNum = m_recvBuffer->GetNextAck();
                ack.hdr.flags = ACK_MASK;
                ack.hdr.dataSize = 0;
                ack.hdr.seqNum = 0;
                ack.hdr.advWindow = m_recvBuffer->GetWindow();
                SendPacket(ack);
                m_ackSent++;
                lpTimer = chrono::steady_clock::now();
                lastAck = ack.hdr.ackNum;
            }
        }
    } while (pkt != NULL);
}

void CTBDevice::UpdateRecv()
{
    bool recv = true;
    do
    {
        Packet *newPkt = new Packet;
        recv = RecvPacket(*newPkt, 10);
        if (recv)
        {
            if (newPkt->hdr.flags & ACK_MASK)
                m_ackRecv++;
            m_sendBuffer->UpdateWithHeader(newPkt->hdr);
            if (newPkt->hdr.dataSize > 0)
            {
                m_packetsRecv++;
                if (!m_recvBuffer->InsertPacket(newPkt))
                {
                    //cout << "Insert fail" << endl;
                    delete newPkt;
                }
            }
            else
            {
                //cout << "Deleting" << endl;
                delete newPkt;
            }
        }

    } while (recv);

    m_sendBuffer->Clean();
}

/*
Update
Updates the buffers as necessary
*/
bool CTBDevice::Update(bool printDebug)
{
    static uint32_t count = 0;

    UpdateSend(printDebug);
    UpdateRecv();

    if (count % 500 == 0 && printDebug)
    {
        Print();
        count = 0;
    }
    count++;
    return true;
}

/*
SendData
Adds a new packet to the queue
*/
bool CTBDevice::SendData(const char *data, uint32_t size)
{
    if (m_sendBuffer->Write(data, size) > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*
RecvData
retrieves a the next packet.
*/
uint32_t CTBDevice::RecvData(char *data, uint32_t size)
{
    return m_recvBuffer->Read(data, size);
}

void CTBDevice::Print()
{
    cout << endl
         << "Stats:" << endl;
    cout << "Data Packets Sent: " << m_packetsSent << endl;
    cout << "Data Packets Recv: " << m_packetsRecv << endl;
    cout << "Ack recv: " << m_ackRecv << endl;
    cout << "Keep alive ack sent: " << m_ackSent << endl;
    cout << "My window: " << m_recvBuffer->GetWindow() << endl;
    cout << "Peer window: " << m_sendBuffer->m_effWindow << endl;
    cout << "My send buffer size: " << m_sendBuffer->m_totalSize << "/" << m_sendBuffer->m_maxSize << endl;
    cout << "My recv buffer size: " << m_recvBuffer->m_totalSize << "/" << m_recvBuffer->m_maxSize << endl;
    cout << "Last ack sent: " << m_recvBuffer->GetNextAck() << endl;
    cout << "Receive fault count " << m_recvBuffer->m_faultCount << endl
         << endl;

   m_sendBuffer->Print();

  //  m_recvBuffer->Print();
}
