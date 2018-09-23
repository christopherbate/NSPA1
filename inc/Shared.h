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

#define FILES_PATH_CLIENT "./"
#define FILES_PATH_SERVER "./"

const string helpMsg = "Commands: put [filename], get [filename], delete [filename], ls, quit";

/*
ParseCmd
Requires: token
Modifies: None
Returns: CmdType if token is a valid cmd, otherwise "UNK" cmd
*/
CmdType ParseCmd(string token);

// Tokenize with spaces.
std::vector<string> TokenizeInput(string input);

bool ExecuteCMD(string &cmd, string &res);



#endif