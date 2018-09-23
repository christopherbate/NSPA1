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
    runner.AddTest("Test handshake", TestCTBDeviceHandshake);
    runner.AddTest("Test send buffer",TestSendBuffer);
    runner.AddTest("Test recv buffer.",TestRecvBuffer);
    runner.AddTest("Test data sending with CTB Protocol",TestCTBDeviceData);
    //runner.AddTest("Test Update Func",TestUpdateSend);
    //runner.AddTest("Test update recv",TestUpdateRecv);
    runner.AddTest("Test send file", TestSendFile);
    runner.AddTest("Test ls",TestLS);
    runner.Run();

    return 0;
}