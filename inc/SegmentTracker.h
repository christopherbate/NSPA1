#ifndef _SEGMENT_TRACKER_H
#define _SEGMENT_TRACKER_H

#include <vector>
#include <iostream>
#include <tuple>

class SegmentTracker
{
  public:
    SegmentTracker()
    {
    }

    /*
    AddByte
    Inserts a new byte into the list, and returns 
    the number following the last contigious block
    */
    void AddByte(uint32_t index)
    {
        // Empty case
        if (m_segments.size() == 0)
        {
            m_segments.push_back(std::make_tuple(index, index));
            return;
        }

        auto it = m_segments.begin();
        for (it; it != m_segments.end(); it++)
        {
            // Check for repeat.
            if(index >= std::get<0>(*it) && 
               index <= std::get<1>(*it))
               return;

            if (std::get<0>(*it) > index)
            {
                break;
            }
        }

        // Check for beginning case.
        if (it == m_segments.begin())
        {
            auto &element = std::get<0>(*it);
            if (index == element - 1)
            {
                element = index;
            }
            else
            {
                m_segments.insert(it, std::make_tuple(index, index));
            }
        }
        else if (it == m_segments.end())
        {
            // End case
            auto &element = std::get<1>(*prev(it));
            if (index == element + 1)
            {
                element = index;
            }
            else
            {
                m_segments.push_back(std::make_tuple(index, index));
            }
        }
        else
        {
            // Middle case.
            auto &lower = std::get<1>(*prev(it));
            auto &upper = std::get<0>(*it);
            // Two connected cases.
            if (index == upper - 1 && index == lower + 1)
            {
                lower = std::get<1>(*it);
                m_segments.erase(it);
            }
            else if (index == upper - 1)
            {
                upper = index;
            }
            else if (index == lower + 1)
            {
                lower = index;
            }
            else
            {
                m_segments.insert(it, std::make_tuple(index, index));
            }
        }
    }

    /*
    GetFirstSegSize
    */
    uint32_t GetFirstSegSize()
    {
        if (m_segments.size() == 0)
        {
            return 0;
        }

        return std::get<1>(m_segments[0]) - std::get<0>(m_segments[0])+1;
    }

    void PrintSegments()
    {        
        for(auto it = m_segments.begin(); it != m_segments.end(); it++)
        {
            std::cout << "(" << std::dec<<std::get<0>(*it) <<"," << std::dec<<std::get<1>(*it)<<"), ";
        }
        std::cout << std::endl;
    }

    void ClearSegments()
    {
        m_segments.clear();
    }

    std::tuple<uint32_t,uint32_t> GetFirstSegment(){
        return m_segments[0];
    }

    uint32_t GetFirstGapIndex()
    {
        if(m_segments.size()==0)
            return 0;
        return std::get<1>(m_segments[0])+1;
    }

  private:
    std::vector<std::tuple<uint32_t, uint32_t>> m_segments;
    std::tuple<uint32_t, uint32_t> m_nextDataBlock;
};

#endif