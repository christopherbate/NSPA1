#include "SendBuffer.h"

SendBuffer::SendBuffer()
{
    m_maxSize = 512000;
    m_lastAckRecv = 0;
    m_lastSeqSent = 0;
    m_nextSeqToSend = 1;
    m_currSeqNum = 1;
    m_totalSize = 0;
    m_effWindow = 512000;
    m_sizeInFlight = 0;
}

SendBuffer::~SendBuffer()
{
    clear();
}

/*
    When the device receives a packet, it calls this in order
    to update last ack
*/
void SendBuffer::UpdateWithHeader(CTBDevice::ProtocolHeader &header)
{
    static uint32_t sameCounter = 0;
    static uint32_t last = 0;

    m_lock.lock();

    // Check for ack flag
    if (header.flags & CTBDevice::ACK_MASK)
    {
        // acknum-1 is the true ack, ack is "next expected byte"
        if (m_lastAckRecv < (header.ackNum - 1))
        {
            // We update to the new consolidated acknowledgment
            m_sizeInFlight -= (header.ackNum-1)-m_lastAckRecv;
            m_lastAckRecv = header.ackNum - 1;
            sameCounter = 0;         
            last = header.ackNum;               
        }
        else
        {
            // We've received this ack before.
            if (last == header.ackNum)
            {
                // Increase the counter
                sameCounter++;

                // Likely that packet is lost!
                // We've received too many times! Retransmit!
                if (sameCounter >= 2)
                {                    
                    sameCounter = 0;                    
                    // Retransmit in flight bits starting with this ackNum(expNum)
                    m_nextSeqToSend = header.ackNum;
                }
            }
            else
            {
                // Update which one we've received.
                last = header.ackNum;
                sameCounter = 0;
            }
        }
    }

    // Set the effective window
    m_effWindow = header.advWindow;


    m_lock.unlock();
}

void SendBuffer::clear()
{
    m_buffer.clear();
    m_maxSize = 512000;
    m_lastAckRecv = 0;
    m_lastSeqSent = 0;
    m_nextSeqToSend = 1;
    m_currSeqNum = 1;
    m_sizeInFlight = 0;
    m_totalSize = 0;
    m_effWindow = 512000;
}

void SendBuffer::Clean()
{
    if (m_buffer.empty())
        return;

    m_lock.lock();
    // Eliminate all elements in the list
    // with seq number less than last ack and sent
    do
    {
        auto it = m_buffer.begin();

        if (it->hdr.seqNum + it->hdr.dataSize - 1 <= m_lastAckRecv &&
            it->hdr.seqNum + it->hdr.dataSize - 1 <= m_lastSeqSent)
        {
            m_totalSize -= it->hdr.dataSize;
            //m_sizeInFlight -= it->hdr.dataSize;
            m_buffer.erase(it);
        }
        else
        {
            break;
        }
    } while (m_buffer.size() > 0);
    m_lock.unlock();
}

CTBDevice::Packet *SendBuffer::GetNextAvail()
{
    m_lock.lock();
    list<CTBDevice::Packet>::iterator it;
    for (it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        // Not sent, not ack'd
        if ((it->hdr.seqNum + it->hdr.dataSize - 1) > m_lastAckRecv && (it->hdr.seqNum) == m_nextSeqToSend)
        {
            m_lock.unlock();
            return &(*it);
        }
    }
    m_lock.unlock();
    return NULL;
}

/*
    Mark sent increase size of bytes in flight and 
*/
void SendBuffer::MarkSent(CTBDevice::ProtocolHeader &header)
{
    // We only update bytes in flight if this wasn't already sent.
    if (header.seqNum + header.dataSize - 1 > m_lastSeqSent)
    {
        m_lastSeqSent = header.seqNum + header.dataSize - 1;      
        m_sizeInFlight += header.dataSize;
    } 

    if(header.seqNum == m_nextSeqToSend){
        m_nextSeqToSend = header.seqNum+header.dataSize;
    }
}

void SendBuffer::Print(bool metaOnly)
{
    cout << "SendBuffer" << endl;
    cout << "LastAck: " << m_lastAckRecv << endl;
    cout << "LastSent: " << m_lastSeqSent << endl;
    cout << "MaxSize: " << m_maxSize << endl;
    cout << "CurrSeq: " << m_currSeqNum << endl;
    cout << "TotalSize: " << m_totalSize << endl;
    cout << "#Elemnts: " << m_buffer.size() << endl;
    cout << "Inflight: " << m_sizeInFlight << endl;
    cout << "Next send: "<<m_nextSeqToSend <<endl;
    cout << "Sent and Ack'd:" << endl;
    for (auto it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        if ((it->hdr.seqNum + it->hdr.dataSize - 1) <= m_lastAckRecv && (it->hdr.seqNum + it->hdr.dataSize - 1) <= m_lastSeqSent)
        {
            std::cout << "Seq: " << it->hdr.seqNum << " Size: " << it->hdr.dataSize << endl;
            if (!metaOnly)
                std::cout << string(&it->data[0], it->hdr.dataSize) << endl;
        }
    }
    cout << endl
         << "Sent and not Ack'd:" << endl;
    for (auto it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        if (it->hdr.seqNum + it->hdr.dataSize - 1 <= m_lastSeqSent && it->hdr.seqNum + it->hdr.dataSize - 1 > m_lastAckRecv)
        {
            std::cout << "Seq: " << it->hdr.seqNum << " Size: " << it->hdr.dataSize << endl;
            if (!metaOnly)
                std::cout << string(&it->data[0], it->hdr.dataSize) << endl;
        }
    }
    cout << endl
         << "Not sent, not ack'd: " << endl;
    for (auto it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        if (it->hdr.seqNum + it->hdr.dataSize - 1 > m_lastSeqSent && it->hdr.seqNum + it->hdr.dataSize - 1 > m_lastAckRecv)
        {
            std::cout << "Seq: " << it->hdr.seqNum << " Size: " << it->hdr.dataSize << endl;
            if (!metaOnly)
                std::cout << string(&it->data[0], it->hdr.dataSize) << endl;
        }
    }
}

uint32_t SendBuffer::Write(const char *data, uint32_t size)
{
    while (size + m_totalSize > m_maxSize)
    {
        //busy wait
    }
    m_lock.lock();

    CTBDevice::Packet pkt;
    pkt.hdr.seqNum = m_currSeqNum;
    pkt.hdr.ackNum = 0;
    pkt.hdr.flags = 0;
    pkt.hdr.advWindow = PACKET_SIZE;
    pkt.hdr.dataSize = size;
    pkt.data.clear();
    if (size > 0)
        pkt.data.insert(pkt.data.end(), &data[0], &data[size]);
    m_buffer.push_back(pkt);
    m_totalSize += size;
    m_currSeqNum = (m_currSeqNum + size) % (uint32_t)0xFFFFFFFF;
    if (m_currSeqNum < pkt.hdr.seqNum)
        cout << "SendData : Seq roll over. old " << pkt.hdr.seqNum << " new " << m_currSeqNum << endl;
    m_lock.unlock();

    return size;
}