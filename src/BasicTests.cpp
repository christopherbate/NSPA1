#include <thread>

#include "Request.h"
#include "Response.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include "BasicTests.h"
#include "CTBDevice.h"
#include "SocketDevice.h"
#include "Shared.h"

using namespace std;

bool TestLS()
{
    string test = ReadLS();
    if (test.length() == 0)
    {
        throw runtime_error("LS failed");
    }
    cout << test << endl;
    return true;
}

bool TestSendFile()
{
    CTBDevice client;
    client.CreateDevice();
    CTBDevice s;
    s.CreateDevice();
    bool cancel = false;

    auto server = [&cancel, &s]() {
        s.Listen("8080", cancel);

        while (!cancel)
        {
            //this_thread::sleep_for(chrono::milliseconds(10));
            s.Update(false);
        }
    };

    auto clientFn = [&cancel, &client]() {
        cout << "Connecting" << endl;
        if (client.ActiveConnect("localhost", "8080", 1000))
        {
            while (!cancel)
            {
                //this_thread::sleep_for(chrono::milliseconds(10));
                client.Update(true);
            }
        }
    };

    auto serverCn = [&s, &cancel]() {
        while (!cancel)
        {

            char buffer[256];
            uint32_t size;
            size = s.RecvData(buffer, 256);
            if (size > 0)
            {
                RequestedDetails req;
                ResponseOpt opt;
                if (ParseRequest(s, buffer, size, req))
                {
                    if (req.type == GET)
                    {
                        opt.isFile = true;
                        opt.filename = req.filename;
                        opt.prefix = "./files_server/";
                        SendResponse(s, opt);
                    }
                    else if (req.type == PUT)
                    {
                        if (RecvFile(s, "./files_server/", req.filename, req.bodySize))
                        {
                            opt.header = "ok";
                        }
                        else
                        {
                            opt.header = "fail";
                        }
                        opt.isFile = false;
                        opt.msg = "";
                        SendResponse(s, opt);
                    }
                    else if (req.type == LS)
                    {
                        string lsResult = ReadLS();
                        opt.isFile = false;
                        opt.msg = lsResult;
                        opt.header = "ok " + to_string(lsResult.length());
                        SendResponse(s, opt);
                    }
                    else if (req.type == DELETE)
                    {
                        string cmd = "rm ./" + req.filename;
                        string rmRes;
                        if (ExecuteCMD(cmd, rmRes))
                        {
                            opt.isFile = false;
                            opt.header = "ok";
                            opt.msg = "";
                            SendResponse(s, opt);
                        }
                        else
                        {
                            opt.header = "fail";
                            opt.isFile = false;
                            opt.msg = "";
                            SendResponse(s, opt);
                        }
                    }
                }
                else
                {
                    // Send reset
                }
            }
        }
    };

    cout << "Launching server thread" << endl;
    std::thread listenThread(server);
    cout << "Launching client thread" << endl;

    std::thread clientThread(clientFn);

    std::this_thread::sleep_for(chrono::milliseconds(1000));

    // Recv file
    cout << "Launching server recv thread" << endl;
    std::thread serverThread(serverCn);

    std::vector<string> responseTokens;
    string filename = "test0.txt";
    string prefix = "./files_client/";

    SendFile(client, prefix,filename,responseTokens);

    cout<<responseTokens[0]<<endl;

    std::this_thread::sleep_for(chrono::milliseconds(1000));
    cout << "Done" << endl;

    s.Print();

    client.Print();

    cancel = true;
    serverThread.join();
    clientThread.join();
    listenThread.join();

    return true;
}

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

bool TestCTBDeviceHandshake()
{
    CTBDevice client;
    if (!client.CreateDevice())
    {
        throw runtime_error("Failed to create ctb device.");
    }

    bool cancel = false;

    auto server = [&cancel]() {
        CTBDevice s;
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

bool TestCTBDeviceData()
{
    CTBDevice client;
    if (!client.CreateDevice())
    {
        throw runtime_error("Failed to create ctb device.");
    }

    bool cancel = false;

    auto server = [&cancel]() {
        CTBDevice s;
        if (!s.CreateDevice())
        {
            throw runtime_error("Failed to server");
        }
        s.Listen("8080", cancel);
        char buffer[256];
        while (!cancel)
        {
            this_thread::sleep_for(chrono::milliseconds(10));
            s.Update();
            uint32_t size = s.RecvData(buffer, 256);
            if (size > 0)
            {
                buffer[size] = '\0';
                cout << buffer << endl;
            }
        }
    };

    std::thread listenThread(server);

    // connect.
    if (!client.ActiveConnect("localhost", "8080", 1000))
    {
        throw runtime_error("Could not connect");
    }

    for (auto i = 0; i < 10; i++)
    {
        string data = "Msg#" + std::to_string(i);
        client.SendData(data.c_str(), data.length());
    }
    client.Update();

    this_thread::sleep_for(chrono::milliseconds(1000));
    cancel = true;

    listenThread.join();

    return true;
}

bool TestUpdateSend()
{
    CTBDevice client;
    if (!client.CreateDevice())
    {
        throw runtime_error("Failed to create ctb device.");
    }

    bool cancel = false;

    auto server = [&cancel]() {
        CTBDevice s;
        if (!s.CreateDevice())
        {
            throw runtime_error("Failed to server");
        }
        s.Listen("8080", cancel);
        char buffer[256];
        while (!cancel)
        {
            this_thread::sleep_for(chrono::milliseconds(10));
            s.Update(true);
            uint32_t size = s.RecvData(buffer, 256);
            if (size > 0)
            {
                buffer[size] = '\0';
                cout << buffer << endl;
            }
        }
    };

    std::thread listenThread(server);

    // connect.
    if (!client.ActiveConnect("localhost", "8080", 5000))
    {
        throw runtime_error("Could not connect");
    }

    for (auto i = 0; i < 10; i++)
    {
        string data = "Msg#" + std::to_string(i);
        client.SendData(data.c_str(), data.length());
    }
    client.Update(false);

    this_thread::sleep_for(chrono::milliseconds(5000));
    cancel = true;

    listenThread.join();

    return true;
}

bool TestUpdateRecv()
{
    CTBDevice client;
    if (!client.CreateDevice())
    {
        throw runtime_error("Failed to create ctb device.");
    }

    bool cancel = false;

    auto server = [&cancel]() {
        CTBDevice s;
        if (!s.CreateDevice())
        {
            throw runtime_error("Failed to server");
        }
        s.Listen("8080", cancel);
        char buffer[256];
        while (!cancel)
        {
            this_thread::sleep_for(chrono::milliseconds(10));
            s.Update(false);
            uint32_t size = s.RecvData(buffer, 256);
            if (size > 0)
            {
                buffer[size] = '\0';
                //cout << buffer << endl;
            }
        }
    };

    std::thread listenThread(server);

    // connect.
    if (!client.ActiveConnect("localhost", "8080", 5000))
    {
        throw runtime_error("Could not connect");
    }

    for (auto i = 0; i < 10; i++)
    {
        string data = "Msg#" + std::to_string(i);
        client.SendData(data.c_str(), data.length());
    }
    client.Update(true);

    this_thread::sleep_for(chrono::milliseconds(5000));
    cancel = true;

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
            throw runtime_error("Shouldn't be null");
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

        if (sb.GetInFlightBytes() != 0)
        {
            throw runtime_error("Should have zero inflight bytes");
        }

        sb.Clean();
        if (sb.m_buffer.size() != 0)
        {
            throw runtime_error("should have empty buffer");
        }

        sb.Print();
    }

    return true;
}

bool TestRecvBuffer()
{
    RecvBuffer rb;
    rb.clear();
    string pk1 = "123456789101112131415";
    string pk2 = "GET test.bin";
    string pk3 = "HELLO WORLD";

    auto packetGen = [](uint32_t seqNum, string data) {
        CTBDevice::Packet *pack = new CTBDevice::Packet;
        pack->hdr.seqNum = seqNum;
        pack->hdr.dataSize = data.length();
        pack->data.insert(pack->data.end(), data.c_str(), data.c_str() + data.length());
        return pack;
    };

    if (rb.GetNextAck() != 1)
    {
        throw runtime_error("Next ack default should be 1");
    }

    CTBDevice::Packet *p1 = packetGen(1, pk1);
    CTBDevice::Packet *p2 = packetGen(22, pk2);
    CTBDevice::Packet *p3 = packetGen(34, pk3);

    rb.InsertPacket(p1);

    if (rb.m_buffer.size() != 1)
        throw runtime_error("Size should be 1");
    if (rb.m_totalSize != pk1.length())
        throw runtime_error("Total size incorrect");

    if (rb.m_faultCount != 0)
    {
        throw runtime_error("OOO Faults should be 0");
    }

    rb.Print();

    if (rb.GetNextAck() != 22)
    {
        throw runtime_error("Get nextAck should get 22");
    }

    if (rb.GetWindow() != rb.m_maxSize - pk1.length())
    {
        throw runtime_error("Incorrect window reproted");
    }

    rb.InsertPacket(p3);

    if (rb.m_buffer.size() != 2)
    {
        throw runtime_error("Incorrect size");
    }

    if (rb.m_faultCount != 1)
    {
        throw runtime_error("Fault count should be 1");
    }

    if (rb.GetNextAck() != 22)
    {
        throw runtime_error("Next ack should not have been adjusted.");
    }

    uint32_t res = rb.Read(NULL, 0);

    if (res != 0)
    {
        throw runtime_error("Shouldnt be able to read with null buffer");
    }

    char buffer[256];
    res = rb.Read(buffer, 256);
    if (res != pk1.length())
    {
        throw runtime_error("Incorrect read length");
    }

    res = rb.Read(buffer, 256);
    if (res != 0)
    {
        throw runtime_error("There shouldn't be data available");
    }

    rb.InsertPacket(p2);

    if (rb.InsertPacket(p2) != false)
    {
        throw runtime_error("Shouldn't be able to insert repeat packet");
    }

    if (rb.m_faultCount != 1)
    {
        throw runtime_error("Fault count should not be adjusted");
    }

    if (rb.GetNextAck() != 45)
    {
        throw runtime_error("Incorrect next ack");
    }

    res = rb.Read(buffer, 256);
    if (res != pk2.length())
    {
        throw runtime_error("Should get pk2");
    }
    res = rb.Read(buffer, 256);
    if (res != pk3.length())
    {
        throw runtime_error("Should get pk3");
    }

    if (rb.GetWindow() != rb.m_maxSize)
    {
        throw runtime_error("Should have full window");
    }

    auto pk4 = packetGen(1, pk1);

    if (rb.InsertPacket(pk4) != false)
    {
        throw runtime_error("Shouldn't be able to insert old packet");
    }

    rb.Print();

    return true;
}