#include "SendBuffer.h"

SendBuffer::SendBuffer()
{
    m_maxSize = 131072;
    m_lastAckRecv = 0;
    m_lastSeqSent = 0;
    m_currSeqNum = 1;
    m_totalSize = 0;
}

SendBuffer::~SendBuffer()
{
    clear();
}

void SendBuffer::UpdateWithHeader(CTBDevice::ProtocolHeader header)
{
    m_lock.lock();
    if (header.flags & CTBDevice::ACK_MASK)
    {
        if (m_lastAckRecv < (header.ackNum - 1))
        {
            m_lastAckRecv = header.ackNum - 1;
        }
    }
    m_lock.unlock();
}

void SendBuffer::clear()
{
    m_buffer.clear();
    m_maxSize = 131072;
    m_lastAckRecv = 0;
    m_lastSeqSent = 0;
    m_currSeqNum = 1;
    m_totalSize = 0;
}

void SendBuffer::Clean()
{
    m_lock.lock();
    // Eliminate all elements in the list
    // with seq number less than last ack and sent
    for (auto it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        if (it->hdr.seqNum + it->hdr.dataSize - 1 <= m_lastAckRecv &&
            it->hdr.seqNum + it->hdr.dataSize -1 <= m_lastSeqSent)
        {
            m_totalSize -= it->hdr.dataSize;
            m_buffer.erase(it);            
        }
        else
        {
            break;
        }
    }
    m_lock.unlock();
}

CTBDevice::Packet *SendBuffer::GetNextAvail()
{
    m_lock.lock();
    vector<CTBDevice::Packet>::iterator it;
    for (it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        // Not sent, not ack'd
        if ((it->hdr.seqNum + it->hdr.dataSize - 1) > m_lastAckRecv && (it->hdr.seqNum + it->hdr.dataSize - 1) > m_lastSeqSent)
        {
            m_lock.unlock();
            return &m_buffer[it - m_buffer.begin()];
        }
    }
    m_lock.unlock();
    return NULL;
}

void SendBuffer::MarkSent( CTBDevice::ProtocolHeader &header, bool force)
{
    if(header.seqNum+header.dataSize < m_lastSeqSent && force){
        m_lastSeqSent = header.seqNum+header.dataSize-1;
    } else if (header.seqNum+header.dataSize-1 > m_lastSeqSent){
        m_lastSeqSent = header.seqNum+header.dataSize-1;
    }
}

void SendBuffer::Print()
{
    cout << "SendBuffer" << endl;
    cout << "LastAck: " << m_lastAckRecv << endl;
    cout << "LastSent: " << m_lastSeqSent << endl;
    cout << "MaxSize: " << m_maxSize << endl;
    cout << "CurrSeq: " << m_currSeqNum << endl;
    cout << "TotalSize: " << m_totalSize << endl;
    cout << "Sent and Ack'd:" << endl;
    for (auto it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        if ((it->hdr.seqNum + it->hdr.dataSize - 1) <= m_lastAckRecv && (it->hdr.seqNum + it->hdr.dataSize - 1) <= m_lastSeqSent)
        {
            std::cout << "Seq: " << it->hdr.seqNum << " Size: " << it->hdr.dataSize << endl;
            std::cout << string(&it->data[0], it->hdr.dataSize) << endl;
        }
    }
    cout << endl<<"Sent and not Ack'd:" << endl;
    for (auto it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        if (it->hdr.seqNum + it->hdr.dataSize - 1 <= m_lastSeqSent && it->hdr.seqNum + it->hdr.dataSize - 1 > m_lastAckRecv)
        {
            std::cout << "Seq: " << it->hdr.seqNum << " Size: " << it->hdr.dataSize << endl;
            std::cout << string(&it->data[0], it->hdr.dataSize) << endl;
        }
    }
    cout << endl<<"Not sent, not ack'd: " << endl;
    for (auto it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        if (it->hdr.seqNum + it->hdr.dataSize - 1 > m_lastSeqSent && it->hdr.seqNum + it->hdr.dataSize - 1 > m_lastAckRecv)
        {
            std::cout << "Seq: " << it->hdr.seqNum << " Size: " << it->hdr.dataSize << endl;
            std::cout << string(&it->data[0], it->hdr.dataSize) << endl;
        }
    }
}

uint32_t SendBuffer::Write(const char *data, uint32_t size)
{
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
}