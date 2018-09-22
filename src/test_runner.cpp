#include <iostream>
#include <thread>

#include "BasicTests.h"

#include <functional>
#include <vector>

using namespace std;

class Test
{
  public:
    Test(string name, const std::function<bool()> f) : m_func(f)
    {
        m_name = name;
    }
    bool Run()
    {
        return m_func();
    }
    std::function<bool()> m_func;
    string m_name;
};

class TestRunner
{
  public:
    TestRunner()
    {
    }

    void AddTest(string name, std::function<bool()> f)
    {
        m_tests.push_back(new Test(name, f));
    }

    void Run()
    {
        const string setRed = "\033[1;31m";
        const string setBlue = "\033[1;34m";
        const string reset = "\033[0m";
        uint32_t count = 0;
        uint32_t fail = 0;
        for (auto it = m_tests.begin(); it != m_tests.end(); it++)
        {
            count++;
            cout << setBlue << "Test " << count
                 << "/" << m_tests.size() << ": " << (*it)->m_name << ": " << reset << endl;
            try
            {
                (*it)->Run();
                delete (*it);
            }
            catch (std::runtime_error &error)
            {
                cerr << setRed << "Failed: " << error.what() << reset << endl;
                fail++;
            }
        }
        cout << setBlue << (count - fail) << " out of " << m_tests.size() << " tests passed." << reset << endl;
    }

    std::vector<Test *> m_tests;
};

int main()
{
    TestRunner runner;

    runner.AddTest("Socket creation", TestSocketCreation);
    runner.AddTest("Socket data send/recv", TestSocketData);
    runner.AddTest("Socket timeout", TestTimeout);
    runner.AddTest("Test CTBDevice creation", TestCTBDevice);
    runner.AddTest("Parse Cmd function", TestHelpers);
    runner.AddTest("Test handshake", TestCTBDeviceData);
    runner.AddTest("Test send buffer",TestSendBuffer);

    runner.Run();

    /*
    // 20 - RecvBuffer
    RecvBuffer rb;
    cout << "Recv buffer testing." << endl;
    cout << "Current send buffer: " << endl;
    sb.Print();
    next = sb.GetNextAvail();
    sb.MarkSent(next->hdr, false);
    next2 = sb.GetNextAvail();
    sb.MarkSent(next2->hdr, false);
    sb.Print();

    cout << "Recv buffer:" << endl;

    rb.InsertPacket(next);
    rb.InsertPacket(next2);
    rb.Print();

    rb.clear();
    rb.InsertPacket(next2);
    rb.InsertPacket(next);
    rb.Print();

    cout << endl;

    cout << "Next ack: " << rb.GetNextAck();

    cout << endl
         << endl;

    CTBDevice::Packet *finPkt = new CTBDevice::Packet;
    finPkt->hdr.seqNum = 1;
    finPkt->hdr.dataSize = pk1.length();
    finPkt->data = std::vector<char>(pk1.begin(), pk1.end());

    rb.InsertPacket(finPkt);

    rb.Print();

    char buffer[PACKET_SIZE];
    uint32_t resl = rb.Read(buffer, PACKET_SIZE);
    cout << "Read result: " << string(buffer, resl) << endl;

    rb.Print();

    cout << endl
         << endl;

    FinalTests();

    cout << endl
         << endl;

    // final results.
    int total = 15;
    cerr << (total - failed) << "/" << total << " tests passed." << endl;
    */
}