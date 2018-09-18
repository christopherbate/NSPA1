#include "Shared.h"

#include <thread>

using namespace std;

#define BLOCK_SIZE 131072

void runClient(CTBDevice *client, string host, string port, bool *cancel)
{
    if (client->ActiveConnect(host, port, 1000))
    {
        cout << "Client connected." << endl;
        while (!*cancel)
        {
            client->UpdateSend();
            client->UpdateRecv();           
        }
    }
}

/*
HandlePut
Requires: filename (no paths / allowed), initialized CTBDevice
Modifies: Opens and sends file over device.
Returns: true on success, otherwise false.
*/
bool HandlePut(string filename, CTBDevice &client)
{
    string filepath = FILES_PATH_CLIENT + filename;
    const uint32_t blockSize = 1024;
    char buffer[blockSize];
    cout << "Opening: " << filepath << endl;
    ifstream infile;
    infile.open(filepath);
    if (!infile.is_open())
    {
        cerr << "File does not exist in files directory." << endl;
        return false;
    }
    // Get the size of the file.
    infile.seekg(0,ios::end);
    unsigned long long fileSize = infile.tellg();
    infile.seekg(0,ios::beg);

    // Send the request to server.
    string msg = "put " + filename + " " + std::to_string(fileSize);
    cout <<"Sending : "<< msg << endl;
    client.SendData((char *)msg.c_str(), msg.length());

    while (!infile.eof())
    {
        infile.read(buffer, blockSize);
        client.SendData(buffer, infile.gcount());
    }    

    return true;
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
    if (!client.CreateDevice(argv[1], argv[2]))
    {
        cerr << "Failed to create client device." << endl;
        return -1;
    }

    unsigned long size = BLOCK_SIZE;
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
            HandlePut(inputTokens[1], client);
            break;
        case GET:
            if (inputTokens.size() < 2)
            {
                cerr << "Must specify a file to get." << endl;
                continue;
            }
            break;
        case LS:
            client.SendData("ls", 2);
            break;
        case DELETE:
            if (inputTokens.size() < 2)
            {
                cerr << "Must specify a file to get." << endl;
                continue;
            }
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