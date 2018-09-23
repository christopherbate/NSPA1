#include "Shared.h"


/*
ParseCmd
Requires: token
Modifies: None
Returns: CmdType if token is a valid cmd, otherwise "UNK" cmd
*/
CmdType ParseCmd(string token)
{
    if (token == "get")
        return GET;
    else if (token == "put")
        return PUT;
    else if (token == "delete")
        return DELETE;
    else if (token == "ls")
        return LS;
    else if (token == "exit")
        return EXIT;

    return UNK;
}

// Tokenize with spaces.
std::vector<string> TokenizeInput(string input)
{
    std::vector<string> inputTokens;
    std::stringstream tokenStream(input);
    while (std::getline(tokenStream, input, ' '))
    {
        inputTokens.push_back(input);
    }
    return inputTokens;
}

bool ExecuteCMD(string &cmd, string &res)
{
    char buffer[256];
    FILE *cmdPipe = popen(cmd.c_str(), "r");
    if (cmdPipe == NULL)
    {
        cerr << "Failed to execute " << cmd << endl;
        return false;
    }

    while (!feof(cmdPipe))
    {
        if (fgets(buffer, 256, cmdPipe) > 0)
        {
            res += string(buffer);
        }
    }
    pclose(cmdPipe);

    return true;
}