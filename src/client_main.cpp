#include "client.h"

using namespace std;

#define BLOCK_SIZE 131072

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

    while (!endLoop)
    {
        // Query the user
        string input;
        cout << helpMsg << endl;
        getline(std::cin, input);

        // Tokenize with spaces.
        vector<string> inputTokens;
        stringstream tokenStream(input);
        while (getline(tokenStream, input, ' '))
        {
            inputTokens.push_back(input);
        }

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
            HandlePut(inputTokens[1],client);
            break;
        case GET:

        case LS:

        case DELETE:

        case EXIT:
            endLoop= true;
            break;
        case UNK:
        default:
            cout <<"Invalid command."<<endl;
            break;
        }
    }

    return 0;
}