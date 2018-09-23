#include "Shared.h"
#include "Request.h"
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
        SaveResponseOpt opt;
        std::vector<string> responseHeaderTokens;
        switch (command)
        {
        case PUT:
            if (inputTokens.size() < 2)
            {
                cerr << "Must specify a file to put." << endl;
                continue;
            }
            SendFile(client, FILES_PATH_CLIENT, inputTokens[1], responseHeaderTokens);
            cout << responseHeaderTokens[0]<<endl;
            break;
        case GET:
            if (inputTokens.size() < 2)
            {
                cerr << "Must specify a file to get." << endl;
                continue;
            }
            Request(client, "get " + inputTokens[1], responseHeaderTokens);
            opt.filename = inputTokens[1];
            opt.prefix = "./";
            opt.printDebug = true;
            opt.size = stoull(responseHeaderTokens[1]);
            opt.saveFile = true;
            SaveResponseBody(client, opt);
            break;
        case LS:
            Request(client, "ls", responseHeaderTokens);
            opt.printDebug = true;
            opt.size = stoull(responseHeaderTokens[1]);
            opt.saveFile = false;
            SaveResponseBody(client, opt);
            cout << opt.result << endl;
            break;
        case DELETE:
            if (inputTokens.size() < 2)
            {
                cerr << "Must specify a file to get." << endl;
                continue;
            }
            Request(client, "delete "+inputTokens[1], responseHeaderTokens);
            cout<< responseHeaderTokens[0] <<endl;
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