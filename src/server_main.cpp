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

void runServer(CTBDevice *server, string port, bool *cancel)
{
    if (server->Listen(port, *cancel))
    {
        while (!*cancel)
        {
            server->Update(false);
            //std::this_thread::sleep_for(chrono::milliseconds(10));
        }
    }
}

/*
    HandleServerPut
*/
void HandleServerPut(std::vector<string> &tokens, CTBDevice &server)
{
    uint64_t size = stoull(tokens[2]);
    RecvFile(server, FILES_PATH_SERVER, tokens[1], size, true);
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

    if (!server.CreateDevice())
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

    cout <<"Server exiting"<<endl;

    return 0;
}