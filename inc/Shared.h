#ifndef CTBCLIENT_H_
#define CTBCLIENT_H_

#include "CTBDevice.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

enum CmdType
{
    GET,
    PUT,
    LS,
    EXIT,
    DELETE,
    UNK
};
#define FILES_PATH_CLIENT "./files_client/"
#define FILES_PATH_SERVER "./files_server/"
const string helpMsg = "Commands: put [filename], get [filename], delete [filename], ls, quit";

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

#endif