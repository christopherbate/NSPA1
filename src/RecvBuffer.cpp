#include "RecvBuffer.h"
#include <unistd.h>
#include "CTBDevice.h"
using namespace std;
RecvBuffer::RecvBuffer()
{
    m_nextExpSeq = 1;
    m_totalSize = 0;
    m_maxSize = 512000;
}

RecvBuffer::~RecvBuffer()
{
    cout << "Recv buffer cleaning up."<<endl;
    clear();
    cout << "Recv buffer done cleaning up."<<endl;
}

void RecvBuffer::clear()
{
    m_buffer.clear();
    m_nextExpSeq = 1;
    m_totalSize = 0;
    m_faultCount = 0;
    m_maxSize = 512000;
}

void RecvBuffer::AdjustExpSeq()
{
    for (auto it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        auto pk = *it;
        if ((pk->hdr.seqNum) == m_nextExpSeq)
        {
            m_nextExpSeq = pk->hdr.seqNum + pk->hdr.dataSize;
            m_faultCount = 0;
        }
        else if (pk->hdr.seqNum > m_nextExpSeq)
        {
            m_faultCount++;
            break;
        }
    }
}

bool RecvBuffer::InsertPacket(CTBDevice::Packet *packet)
{
    m_lock.lock();
    if (!m_buffer.empty())
    {
        auto seq = packet->hdr.seqNum;
        list<CTBDevice::Packet *>::iterator it;
        for (it = m_buffer.begin(); it != m_buffer.end(); it++)
        {
            auto nextPak = *it;
            if (seq < nextPak->hdr.seqNum)
            {
                break;
            }
            else if (seq == nextPak->hdr.seqNum)
            {
                cerr << "Repeat packet, ignoring" << endl;
                m_lock.unlock();
                return false;
            }
        }
        m_buffer.insert(it, packet);
    }
    else
    {
        m_buffer.push_back(packet);
    }
    m_totalSize += packet->hdr.dataSize;
    AdjustExpSeq();
    m_lock.unlock();

    return true;
}

uint32_t RecvBuffer::Read(char *data, uint32_t size)
{
    uint32_t res = 0;
    if(m_buffer.empty()){
        return 0;
    }
    m_lock.lock();    
    auto pk = m_buffer.front();        
    if ((pk->hdr.seqNum + pk->hdr.dataSize - 1) < m_nextExpSeq)
    {
        if (size < pk->hdr.dataSize)
            throw runtime_error("Provided buffer is to small to receive packet");        
        std::copy(pk->data.begin(), pk->data.end(), data);
        res = pk->hdr.dataSize;            
        delete pk;
        m_buffer.pop_front();        
        m_totalSize -= res;
    }
    m_lock.unlock();
    return res;
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

void RecvBuffer::Print()
{
    cout << "RecvBuffer" << endl;
    cout << "MaxSize: " << m_maxSize << endl;
    cout << "NextExp: " << m_nextExpSeq << endl;
    cout << "TotalSize: " << m_totalSize << endl;
    cout << "OOO Faults: " << m_faultCount << endl;
    cout << "Received and Ready:" << endl;
    for (auto it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        auto pk = *it;
        if ((pk->hdr.seqNum + pk->hdr.dataSize - 1) < m_nextExpSeq)
        {
            std::cout << "Seq: " << pk->hdr.seqNum << " Size: " << pk->hdr.dataSize << endl;
            std::cout << string(&pk->data[0], pk->hdr.dataSize) << endl;
        }
    }
    cout << endl
         << "Received and not Ready" << endl;
    for (auto it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        auto pk = *it;
        if (pk->hdr.seqNum + pk->hdr.dataSize - 1 > m_nextExpSeq)
        {
            std::cout << "Seq: " << pk->hdr.seqNum << " Size: " << pk->hdr.dataSize << endl;
            std::cout << string(&pk->data[0], pk->hdr.dataSize) << endl;
        }
    }
    cout << endl;
}