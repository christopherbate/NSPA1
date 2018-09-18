#include <iostream>
#include <thread>
#include "../inc/SocketDevice.h"
#include "CTBDevice.h"
#include "Shared.h"
#include "SendBuffer.h"

using namespace std;

string pk1 = "123456789101112131415";
string pk2 = "GET test.bin";
string pk3 = "HELLO WORLD";

void serverFn(bool *cancel)
{
    CTBDevice server;
    server.CreateDevice("8080");
    server.Listen("8080", *cancel);
    char buffer[PACKET_SIZE];
    std::this_thread::sleep_for(chrono::seconds(1));
    server.UpdateRecv();
    uint32_t res = 0;
    do
    {
        res = server.RecvData(buffer, PACKET_SIZE);
        if (res > 0)
            std::cout << "Data: " + string(buffer, res) << " " << std::dec << res << endl;
    } while (res > 0);
    server.DestroyDevice();
}

bool testProtocol()
{
    CTBDevice client;
    client.CreateDevice("localhost", "8080");
    bool cancel = false;
    std::thread serverThread(serverFn, &cancel);
    cout << "Launched server threads" << endl;
    if (client.ActiveConnect("localhost", "8080", 10))
    {
        cout << "Client Connected" << endl;
        client.SendData(pk1.c_str(), pk1.length());
        client.SendData(pk2.c_str(), pk2.length());
        client.SendData(pk3.c_str(), pk3.length());
        client.UpdateSend();
    }
    else
    {
        cout << "Failed" << endl;
        return false;
    }

    this_thread::sleep_for(chrono::milliseconds(1000));
    client.UpdateRecv(); // To Receive the acks
    client.UpdateRecv(); // To Receive the acks
    client.UpdateRecv(); // To Receive the acks
    this_thread::sleep_for(chrono::milliseconds(5000));
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

    // 13 - update recv.

    // 14 - 3-way Handshake
    /*if(!testProtocol()){
        cout << "14 - Handshake failed"<<endl;
        failed++;
    }*/

    // 15 - Send Buffers
    SendBuffer sb;
    CTBDevice::Packet *next = NULL;
    sb.Write(pk1.c_str(), pk1.length());
    sb.Write(pk2.c_str(), pk2.length());
    sb.Write(pk3.c_str(), pk3.length());
    sb.Clean();
    sb.Print();
    next = sb.GetNextAvail();
    if (next == NULL)
    {
        failed++;
        cerr << "Should be a packet available to send." << endl;
    }
    else
    {
        client.PrintHeader(next->hdr);
    }

    cout << endl;

    // 16 - update with header (1)
    CTBDevice::ProtocolHeader hdr;
    hdr.flags = CTBDevice::ACK_MASK;
    hdr.ackNum = 1;

    // This shouldn't update anything.
    sb.UpdateWithHeader(hdr);
    if (sb.m_lastAckRecv != 0)
    {
        cerr << "Ack should not update" << endl;
        failed++;
    }
    sb.Clean();
    sb.Print();
    next = sb.GetNextAvail();
    if (next == NULL)
    {
        failed++;
        cerr << "Should be a packet available to send." << endl;
    }
    else
    {
        client.PrintHeader(next->hdr);
    }
    cout << endl;

    //17 - update with header (2)
    // Update last ack, but not everything should stay
    // "not sent"
    hdr.ackNum = 2;
    sb.UpdateWithHeader(hdr);
    sb.Clean();
    sb.Print();
    next = sb.GetNextAvail();
    if (next == NULL)
    {
        failed++;
        cerr << "Should be a packet available to send." << endl;
    }
    else
    {
        client.PrintHeader(next->hdr);
    }

    cout << endl;

    // 18 - mark as sent
    // Should change first packet to sent, not ack'd.
    next = sb.GetNextAvail();
    sb.MarkSent(next->hdr, false);
    sb.Print();
    sb.Clean();
    CTBDevice::Packet *next2 = sb.GetNextAvail();
    if (next2 == NULL)
    {
        failed++;
        cerr << "Should be a packet available to send." << endl;
    }
    else
    {
        client.PrintHeader(next->hdr);
    }

    // 19 - Ack and Clean
    cout << endl;
    hdr.flags = CTBDevice::ACK_MASK;
    hdr.ackNum = next->hdr.seqNum+next->hdr.dataSize;
    sb.UpdateWithHeader(hdr);
    sb.Print();
    sb.Clean();
    cout << endl;
    sb.Print();

    // 19 - mark as acknowledged
    // Should change first packet to sent, and ack'd

    // final results.
    int total = 15;
    cerr << (total - failed) << "/" << total << " tests passed." << endl;
}