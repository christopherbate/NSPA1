#include <thread>

#include "RecvBuffer.h"
#include "SendBuffer.h"
#include "BasicTests.h"
#include "CTBDevice.h"
#include "SocketDevice.h"
#include "Shared.h"

using namespace std;

bool TestSocketCreation()
{
    SocketDevice sock;

    if (sock.CreateSocket("www.google.com", "80") == false)
    {
        throw runtime_error("Couldnt create active udp socket");
    }

    sock.CloseSocket();

    if (sock.CreateSocket("host", "80") == true)
    {
        throw runtime_error("Shouldn't create socket with bad host");
    }

    sock.CloseSocket();

    sock.CreateSocket("www.google.com", "80");
    if (sock.CreateSocket("www.google.com", "80") == true)
    {
        throw runtime_error("Shouldn't be able to create socket twice");
    }

    sock.CloseSocket();

    if (sock.CreateSocket("8080") == false)
    {
        throw runtime_error("Couldn't create local listen UDP socket");
    }

    sock.CloseSocket();

    if (sock.CreateSocket("8080") && sock.CreateSocket("8080"))
    {
        throw runtime_error("Shouldn't be able to create listen socket twice");
    }

    return true;
}

bool TestSocketData()
{
    SocketDevice client;
    SocketDevice server;

    client.CreateSocket("localhost", "8080");

    server.CreateSocket("8080");

    string msg = "data";

    if (client.BlockingSend(msg.c_str(), msg.length()) != msg.length())
        throw runtime_error("Short write");

    if (!server.SetRecvTimeout(10))
    {
        throw runtime_error("Failed to set timeout");
    }

    char buffer[256];
    string address;
    uint32_t size = 256;
    if (server.BlockingRecv(buffer, size, address) != msg.length())
        throw runtime_error("Incorrect read size");

    return true;
}

bool TestTimeout()
{
    SocketDevice server;
    if (server.SetRecvTimeout(1000000))
    {
        throw runtime_error("Shouldn't be able to set timeout");
    }
    if (!server.CreateSocket("8080"))
    {
        throw runtime_error("Couldn't create server");
    }
    if (!server.SetRecvTimeout(1000000))
    {
        throw runtime_error("Should be able to set timeout");
    }
    char data[256];
    string addr;
    if (server.BlockingRecv(data, 256, addr) != 0)
    {
        throw runtime_error("Should be zero on timeout");
    }
    if (!server.DidTimeout())
    {
        throw runtime_error("Timeout should be set.");
    }
    return true;
}

bool TestCTBDevice()
{
    CTBDevice client;
    if (!client.CreateDevice())
    {
        throw runtime_error("Failed to create ctb device.");
    }
    client.DestroyDevice();

    // 9 - create client ands server
    if (!client.CreateDevice())
    {
        throw runtime_error("Failed to create device after destruction.");
    }
    if (client.CreateDevice())
    {
        throw runtime_error("Shouldnt be able to create device twice");
    }
    client.DestroyDevice();
    return true;
}

bool TestCTBDeviceData()
{
    CTBDevice client;
    if (!client.CreateDevice())
    {
        throw runtime_error("Failed to create ctb device.");
    }

    auto server = []() {
        CTBDevice s;
        bool cancel;
        if (!s.CreateDevice())
        {
            throw runtime_error("Failed to server");
        }
        s.Listen("8080", cancel);
    };

    std::thread listenThread(server);

    // connect.
    if (!client.ActiveConnect("localhost", "8080", 1000))
    {
        throw runtime_error("Could not connect");
    }

    listenThread.join();

    return true;
}

bool TestHelpers()
{
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
        throw runtime_error("Parse cmd returned incorrect");
    }

    return true;
}

void ServerThread(CTBDevice *device, bool *cancel)
{
    if (device->Listen("8080", *cancel))
    {
        cout << "Server connected" << endl;
        while (!(*cancel))
        {
            device->Update();
            char buffer[PACKET_SIZE];
            uint32_t size;
            size = device->m_recvBuffer->Read(buffer, PACKET_SIZE);
            if (size > 0)
            {
                cout << "Recv: " << string(buffer, size) << endl;
            }
        }
        cout << "Joining." << endl;
    }
}

void FinalTests()
{
    string pk1 = "123456789101112131415";
    string pk2 = "GET test.bin";
    string pk3 = "HELLO WORLD";
    CTBDevice client;
    CTBDevice server;

    server.CreateDevice();

    client.CreateDevice();

    bool cancel = false;

    std::thread t1(ServerThread, &server, &cancel);

    if (client.ActiveConnect("localhost", "8080", 1000))
    {
        cout << "Client connected." << endl;
        if (client.SendData(pk1.c_str(), pk1.length()))
        {
            client.SendData(pk2.c_str(), pk2.length());
            client.SendData(pk3.c_str(), pk3.length());
            cout << "Sent data." << endl;
        }
        cout << "Client updating" << endl;
        client.Update();
        client.Update();
        client.Update();
        client.Update();
    }

    std::this_thread::sleep_for(chrono::seconds(1));
    cancel = true;
    cout << "Ending" << endl;
    t1.join();
}

bool TestSendBuffer()
{
    string pk1 = "123456789101112131415";
    string pk2 = "GET test.bin";
    string pk3 = "HELLO WORLD";
    SendBuffer sb;
    CTBDevice::Packet *next = NULL;
    sb.Write(pk1.c_str(), pk1.length());
    sb.Write(pk2.c_str(), pk2.length());
    sb.Write(pk3.c_str(), pk3.length());
    sb.Clean();
    next = sb.GetNextAvail();
    if (next == NULL)
    {
        throw runtime_error("Failed to get next packet");
    }
    else
    {
        if (next->hdr.seqNum != 1)
        {
            throw runtime_error("Should have seq 1");
        }
        if (next->hdr.dataSize != pk1.length())
        {
            throw runtime_error("Should have pk1 length");
        }

        sb.MarkSent(next->hdr);

        // Set bytes in flight
        if (sb.GetInFlightBytes() != pk1.length())
        {
            throw runtime_error("Should update bytes in flight");
        }

        CTBDevice::Packet *next2 = sb.GetNextAvail();
        if (next2 == NULL)
        {
            throw runtime_error("Shoudn't be null");
        }
        sb.MarkSent(next2->hdr);
        sb.MarkSent(next->hdr); // Try to remark send, should fail.
        if (sb.GetInFlightBytes() != pk1.length() + pk2.length())
        {
            throw runtime_error("Two packets should be in flight");
        }

        // Ack third packet.
        CTBDevice::Packet ack;
        ack.hdr.flags = CTBDevice::ACK_MASK;
        ack.hdr.ackNum = next->hdr.seqNum + next->hdr.dataSize;
        sb.UpdateWithHeader(ack.hdr);
        if (sb.GetInFlightBytes() != pk2.length())
        {
            throw runtime_error("Inflight bytes not being updated.");
        }

        // Clean buffer
        sb.Clean();
        if (sb.m_buffer.size() != 2)
        {
            throw runtime_error("Should update buffer.");
        }

        // Get the last packet.
        next = sb.GetNextAvail();
        if (next == NULL)
        {
            throw runtime_error("Shouldn't be null");
        }
        if (next->hdr.dataSize != pk3.length())
        {
            throw runtime_error("Incorrect third packet");
        }
        sb.MarkSent(next->hdr);
        if (sb.GetInFlightBytes() != pk2.length() + pk3.length())
        {
            throw runtime_error("Inflight should be pk2 and pk3");
        }

        // Should get null.
        if (sb.GetNextAvail() != NULL)
            throw runtime_error("Should get null");

        // Repeat the first ack another two times, should reset to retrasnmit pk2 and pk3
        sb.UpdateWithHeader(ack.hdr);
        sb.UpdateWithHeader(ack.hdr);

        next = sb.GetNextAvail();
        if (next == NULL)
        {
            throw runtime_error("Next avail should be packet 2");
        }
        if (next->hdr.dataSize != pk2.length())
        {
            throw runtime_error("Should retransmit pk2");
        }
        next = sb.GetNextAvail();
        if (next->hdr.dataSize != pk2.length())
        {
            throw runtime_error("Should retransmit pk2");
        }

        if (sb.GetInFlightBytes() != pk2.length() + pk3.length())
        {
            throw runtime_error("Inflight should be pk2 and pk3");
        }

        sb.MarkSent(next->hdr);

        if (sb.GetInFlightBytes() != pk2.length() + pk3.length())
        {
            throw runtime_error("Inflight should be pk2 and pk3");
        }

        next = sb.GetNextAvail();
        if (next == NULL)
        {
            throw runtime_error("Next avail should be packet 3");
        }
        if (next->hdr.dataSize != pk3.length())
        {
            throw runtime_error("Should retransmit pk3");
        }
        sb.MarkSent(next->hdr);
        // Should get null.
        if (sb.GetNextAvail() != NULL)
            throw runtime_error("Should get null");
        sb.Print();
        // Ack the last two packets.
        ack.hdr.ackNum = 34;
        sb.UpdateWithHeader(ack.hdr);
        ack.hdr.ackNum = 45;
        sb.UpdateWithHeader(ack.hdr);

        if(sb.GetInFlightBytes()!=0){
            throw runtime_error("Should have zero inflight bytes");
        }

        sb.Clean();
        if(sb.m_buffer.size()!=0){
            throw runtime_error("should have empty buffer");
        }

        sb.Print();
    }

    return true;
}

bool TestRecvBuffer()
{   
    RecvBuffer rb;
    return true;
}