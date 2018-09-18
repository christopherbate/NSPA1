#include "CTBDevice.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <fstream>
#include "Shared.h"
using namespace std;

#define FILES_DIR "./"

#define BLOCK_SIZE 131072

void runServer(CTBDevice *server, string port, bool *cancel)
{
    if (server->Listen(port, *cancel))
    {
        while (!*cancel)
        {
            server->UpdateSend();
            server->UpdateRecv();            
        }
    }
}

/*
    HandleServerPut
*/
void HandleServerPut(std::vector<string> &tokens, CTBDevice &server)
{
    char buffer[PACKET_SIZE];
    unsigned long long fileSize = stoull(tokens[2]);
    cout << "Receiving file of size " << fileSize << endl;
    ofstream out;
    unsigned long long total = 0;
    out.open(FILES_PATH_SERVER + tokens[1]);
    bool write = true;
    if (!out.is_open())
    {

        cerr << "Server : Could not open file for put." << endl;
        write = false;
    }
    unsigned int loopCnt = 0;
    while (total < fileSize)
    {
        uint32_t size = server.RecvData(buffer, PACKET_SIZE);
        if (size > 0)
        {
            if (write)
                out.write(buffer, size);
            total += size;
            loopCnt++;
            if(loopCnt%10==0){
                cout << "Transfer " << total << "/" << fileSize << endl;
            }
        }
    }
    cout << "Transfer complete " << total << "/" << fileSize << endl;
    cout << "Transfer took " << loopCnt << " packets." << endl;
    cout << "Server window "<<server.m_advWindow<<endl;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " [port]" << endl;
        return -1;
    }

    CTBDevice server;
    bool cancel = false;

    if (!server.CreateDevice(argv[1]))
    {
        cerr << "Error creating server." << endl;
        return -1;
    }

    std::thread serverThread(runServer, &server, argv[1], &cancel);
    cout << "Server running on " << argv[1] << endl;
    char buffer[PACKET_SIZE];
    while (!cancel)
    {
        uint32_t size = server.RecvData(buffer, PACKET_SIZE);
        if (size > 0)
        {
            string input = string(buffer, size);
            cout << "Received: " << input << endl;
            std::vector<string> tokens = TokenizeInput(input);
            CmdType cmd = ParseCmd(tokens[0]);
            switch (cmd)
            {
            case GET:
                cout << "Received GET" << endl;
                break;
            case PUT:
                HandleServerPut(tokens, server);
                break;
            default:
                break;
            }
        }
        else
        {
            std::this_thread::sleep_for(chrono::seconds(1));
        }
    }

    return 0;
}