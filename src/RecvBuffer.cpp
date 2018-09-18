#include "RecvBuffer.h"

RecvBuffer::RecvBuffer()
{
    m_nextExpSeq = 1;
    m_totalSize = 0;
    m_maxSize = 131072;
}

RecvBuffer::~RecvBuffer()
{
}

void RecvBuffer::clear()
{
    m_nextExpSeq = 1;
    m_totalSize = 0;
    m_maxSize = 131072;
}

void RecvBuffer::InsertPacket(CTBDevice::Packet *packet)
{
    m_lock.lock();
    list<CTBDevice::Packet *>::iterator it;
    for (it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
    }
    m_lock.unlock();
    m_lock.unlock();
}

uint32_t RecvBuffer::Read(char *buffer, uint32_t *size)
{
}

uint32_t RecvBuffer::GetNextAck()
{
    uint32_t val;
    m_lock.lock();
    val = m_nextExpSeq;
    m_lock.unlock();
    return val;
}

uint32_t RecvBuffer::GetWindow()
{
    if (m_totalSize < m_maxSize)
        return m_maxSize - m_totalSize;
    else
        return 0;
}