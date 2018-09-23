#include "Shared.h"

#include <thread>

using namespace std;

void runClient(CTBDevice *client, string host, string port, bool *cancel)
{
    while (!(*cancel))
    {
        if (client->ActiveConnect(host, port, 1000))
        {
            cout << "Client connected." << endl;
            while (!*cancel)
            {
                client->Update(false);
                //std::this_thread::sleep_for(chrono::milliseconds(10));
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "Usage: " << argv[0] << " [hostname] [port]" << endl;
        return 0;
    }

    // Create our client device
    CTBDevice client;
    if (!client.CreateDevice())
    {
        cerr << "Failed to create client device." << endl;
        return -1;
    }

    CmdType command;
    bool endLoop = false;
    std::thread clientThread(runClient, &client, argv[1], argv[2], &endLoop);
    while (!endLoop)
    {        
        // Query the user
        string input;
        cout << helpMsg << endl;
        getline(std::cin, input);

        vector<string> inputTokens = TokenizeInput(input);

        // Parse the query.
        if (inputTokens.size() < 1)
        {
            cout << "Input not valid." << endl;
            continue;
        }

        // Parse the command
        command = ParseCmd(inputTokens[0]);

        switch (command)
        {
        case PUT:
            if (inputTokens.size() < 2)
            {
                cerr << "Must specify a file to put." << endl;
                continue;
            }
            SendFile(client, FILES_PATH_CLIENT, inputTokens[1]);            
            break;
        case GET:
            if (inputTokens.size() < 2)
            {
                cerr << "Must specify a file to get." << endl;
                continue;
            }
            //HandleGet(inputTokens[1],client);
            break;
        case LS:
            //HandleLs(client);
            break;
        case DELETE:
            if (inputTokens.size() < 2)
            {
                cerr << "Must specify a file to get." << endl;
                continue;
            }
            //HandleDelete(client);
            break;
        case EXIT:
            endLoop = true;
            break;
        case UNK:
        default:
            cout << "Invalid command." << endl;
            break;
        }
    }

    clientThread.join();

    return 0;
}