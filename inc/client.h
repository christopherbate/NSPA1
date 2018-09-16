#ifndef CTBCLIENT_H_
#define CTBCLIENT_H_

#include "CTBDevice.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

enum CmdType {GET,PUT,LS,EXIT,DELETE,UNK};
#define FILES_PATH "../files_client/"
const string helpMsg = "Commands: put [filename], get [filename], delete [filename], ls, quit";


/*
ParseCmd
Requires: token
Modifies: None
Returns: CmdType if token is a valid cmd, otherwise "UNK" cmd
*/
CmdType ParseCmd(string token){
    if(token == "get")
        return GET;
    else if(token == "put")
        return PUT;
    else if(token == "delete")
        return DELETE;
    else if (token == "ls")
        return LS;
    else if (token == "exit")
        return EXIT;

    return UNK;
}


/*
HandlePut
Requires: filename (no paths / allowed), initialized CTBDevice
Modifies: Opens and sends file over device.
Returns: true on success, otherwise false.
*/
bool HandlePut(string filename, CTBDevice &client )
{
    string filepath = FILES_PATH + filename;
    char buffer[131072];
    cout << "Opening: " << filepath << endl;
    ifstream infile;
    infile.open(filepath);
    if (!infile.is_open())
    {
        cerr << "File does not exist in files directory." << endl;
        return false;
    }

    // Send the request to server.
    string msg = "PUT " + filename;
    client.SendData((char *)msg.c_str(), msg.length());

    while (!infile.eof())
    {
        infile.read(buffer, 131072);
        client.SendData(buffer, infile.gcount());
    }

    return true;
}


#endif