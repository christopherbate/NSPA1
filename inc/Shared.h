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

// Send file
bool SendFile(CTBDevice &device, string prefix, string filepath)
{
    const uint32_t blockSize = PACKET_SIZE / 2;
    char buffer[blockSize];

    ifstream infile;
    infile.open(prefix + filepath);
    if (!infile.is_open())
    {
        cerr << "File does not exist in files directory." << endl;
        return false;
    }

    // Get the size of the file.
    infile.seekg(0, ios::end);
    unsigned long long fileSize = infile.tellg();
    infile.seekg(0, ios::beg);

    // SEND REQUEST
    string msg = "put " + filepath + " " + std::to_string(fileSize);
    cout << "Sending : " << msg << endl;
    auto timer = chrono::steady_clock::now();
    device.SendData((char *)msg.c_str(), msg.length());

    // SEND FILE
    uint64_t total = 0;
    while (!infile.eof())
    {
        infile.read(buffer, blockSize);                
        if (infile.gcount() > 0)
        {
            device.SendData(buffer, infile.gcount());
            total += infile.gcount();
        }
    }

    auto msec = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - timer).count();
    cout << "Transfer complete " << total << "/" << fileSize << endl;
    cout << "Transfer took " << msec << " milliseconds." << endl;

    return true;
}

bool RecvFile(CTBDevice &device, string prefix, string filename, uint64_t size, bool print = false)
{
    char buffer[PACKET_SIZE];
    cout << "Receiving " << filename << " of size " << size << endl;

    ofstream out;
    uint64_t total = 0;
    out.open(prefix + filename);

    bool write = true;
    if (!out.is_open())
    {

        cerr << "Server : Could not open file for put." << endl;
        write = false;
    }

    unsigned int loopCnt = 0;
    auto timer = chrono::steady_clock::now();
    while (total < size)
    {
        uint32_t recvSize = device.RecvData(buffer, PACKET_SIZE);
        if (recvSize > 0)
        {
            if (write)
                out.write(buffer, recvSize);
            total += recvSize;
            loopCnt++;
            if (loopCnt % 200 == 0 && print)
            {
                cout << "Transfer " << total << "/" << size << endl;
            }
        }
    }
    auto msec = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - timer).count();
    cout << "Transfer complete " << total << "/" << size << endl;
    cout << "Transfer took " << msec << " milliseconds." << endl;

    return true;
}

#endif