#include <iostream>
#include <thread>
#include "../inc/SocketDevice.h"
#include "CTBDevice.h"
#include "client.h"
#include "SegmentTracker.h"

using namespace std;

void serverFn(bool *cancel)
{
    CTBDevice server;
    server.CreateDevice("8080");
    server.Listen("8080", *cancel);

    server.DestroyDevice();
}

bool testProtocol()
{
    CTBDevice client;
    client.CreateDevice("localhost", "8080");
    bool cancel = false;
    std::thread serverThread(serverFn, &cancel);
    cout << "Launched server threads" << endl;
    if (client.ActiveConnect("localhost", "8080",10))
    {
        cout << "Connected" << endl;
    }
    else
    {
        cout << "Failed" << endl;
        return false;
    }
    cancel = true;
    serverThread.join();

    client.DestroyDevice();

    return true;
}

int main()
{
    cout << "Testing... " << endl;
    int failed = 0;

    // 1 - test conn socket creation google.com:80
    SocketDevice Socket;
    SocketDevice Socket2;

    if (Socket.CreateSocket("www.google.com", "80") == false)
    {
        cerr << "Failed 1" << endl;
        failed++;
    }
    Socket.CloseSocket();

    cout << endl;

    // 2 - bad host
    if (Socket.CreateSocket("asdfdaf", "80") == true)
    {
        cerr << "Failed 1" << endl;
        failed++;
    }
    Socket.CloseSocket();

    cout << endl;

    // 3 - socket already open.
    Socket.CreateSocket("www.google.com", "80");
    if (Socket.CreateSocket("www.google.com", "80") == true)
    {
        cerr << "Failed 3" << endl;
        failed++;
    }
    Socket.CloseSocket();

    cout << endl;

    // 4 listen socket on 8080
    if (Socket.CreateSocket("8080") == false)
    {
        cerr << "Failed to create list socket." << endl;
        failed++;
    }
    Socket.CloseSocket();

    cout << endl;

    // 5 Try to listen twice (should fail)
    if (Socket.CreateSocket("8080") == false)
    {
        failed++;
    }
    else
    {
        if (Socket.CreateSocket("8080") == true)
        {
            cerr << "Allowed to create too many listen binds." << endl;
            failed++;
        }
    }
    Socket.CloseSocket();

    cout << endl;

    // 6 Try to send data.
    if (Socket.CreateSocket("8080"))
    {
        if (Socket2.CreateSocket("localhost", "8080"))
        {
            string msg = "stuff";
            if (Socket2.BlockingSend((char *)msg.c_str(), msg.length()) != msg.length())
            {
                cerr << "Short write." << endl;
                failed++;
            }
            else
            {
                cout << "Send successful" << endl;
            }
        }
        else
        {
            cerr << "6- failed to create socket 2" << endl;
            failed++;
        }
    }
    else
    {
        cerr << " 6 failed to create server socket" << endl;
        failed++;
    }
    cout << endl;
    Socket.CloseSocket();
    Socket2.CloseSocket();

    // 7 Try to send and recv data.
    if (Socket.CreateSocket("8080"))
    {
        if (Socket2.CreateSocket("localhost", "8080"))
        {
            string msg = "stuff";
            if (Socket2.BlockingSend((char *)msg.c_str(), msg.length()) != msg.length())
            {
                cerr << "Short write." << endl;
                failed++;
            }
            else
            {
                cout << "Send successful" << endl;
                char buffer[PACKET_SIZE];
                string address;
                unsigned int len = Socket.BlockingRecv(buffer, PACKET_SIZE, address);
                buffer[len] = '\0';
                cout << "Addr:" << address << endl;
                cout << "Len:" << len << endl;
                cout << "Recv: " << buffer << endl;
                if (Socket.DidTimeout())
                {
                    cerr << "7 - Timeout flag should not be set." << endl;
                    failed++;
                }
            }
        }
        else
        {
            cerr << "6- failed to create socket 2" << endl;
            failed++;
        }
    }
    else
    {
        cerr << " 6 failed to create server socket" << endl;
        failed++;
    }
    cout << endl;
    Socket.CloseSocket();
    Socket2.CloseSocket();

    cout << endl;

    //8 - create ctb device
    CTBDevice client;
    if (!client.CreateDevice("localhost", "8080"))
    {
        cerr << "8 Failed to create client" << endl;
        failed++;
    }
    client.DestroyDevice();

    cout << endl;

    // 9 - create client ands server
    CTBDevice server;
    if (!client.CreateDevice("localhost", "8080"))
    {
        cerr << "8 Failed to create client" << endl;
        failed++;
    }
    if (!server.CreateDevice("8080"))
    {
        cerr << "8 Failed to create server" << endl;
        failed++;
    }
    client.DestroyDevice();
    server.DestroyDevice();

    cout << endl;
    cout << endl;

    // 10 - set socket timeout.
    Socket.CreateSocket("8080");
    if (Socket.SetRecvTimeout(1000) == false)
    {
        cerr << "10 - fail to set server timeout." << endl;
        failed++;
    }
    Socket.CloseSocket();

    cout << endl;
    cout << endl;
    cout << endl;

    // 11 - test timeout.
    Socket.CreateSocket("www.google.com", "80");
    if (!Socket.SetRecvTimeout(1000000))
    {
        cerr << "Failed to set timeout" << endl;
        failed++;
    }
    char dummy[100];
    string sdummy;
    if (Socket.BlockingRecv(dummy, 100, sdummy, NULL) != 0)
    {
        failed++;
        cerr << "Return should be 0 on timeout" << endl;
    }
    else
    {
        if (!Socket.DidTimeout())
        {
            failed++;
            cerr << "Timeout flag not set." << endl;
        }
    }
    cout << endl;
    cout << endl;

    // 12 - client input parsing.
    bool res = true;
    if (ParseCmd("put") != PUT)
        res = false;
    if (ParseCmd("ls") != LS)
        res = false;
    if (ParseCmd("get") != GET)
        res = false;
    if (ParseCmd("exit") != EXIT)
        res = false;
    if (ParseCmd("delete") != DELETE)
        res = false;
    if (ParseCmd("blah") != UNK)
        res = false;
    if (!res)
    {
        failed++;
        cerr << "Parse Cmd error.";
    }
    cout << endl;
    cout << endl;

    // 13 - segment tracking.
    SegmentTracker st;
    while (1)
    {
        cout << "Segment tracker testing. Enter byte: ";
        string input;
        std::getline(std::cin, input);
        if (input == "q")
            break;
        unsigned long byte = stoul(input);
        cout << "Entered: " << byte << endl;
        st.AddByte(byte);
        st.PrintSegments();
    }
    cout << endl;
    cout << endl;

    // 14 - 3-way Handshake
    if(!testProtocol()){
        cout << "14 - Handshake failed"<<endl;
        failed++;
    }

    // final results.
    int total = 13;
    cerr << (total - failed) << "/" << total << " tests passed." << endl;
}