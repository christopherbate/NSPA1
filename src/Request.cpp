#include "Request.h"
#include "Shared.h"

/*
Request 

Helper Function
Sends a request given by "request" string using device.
Awaits response (blocking) and returns response as vector of string tokens.
*/
bool Request(CTBDevice &device, string request, std::vector<string> &responseHeaderTokens)
{
    const uint32_t blockSize = PACKET_SIZE * 2;
    char buffer[blockSize];

    // SEND REQUEST
    std::cout << "Requesting : " << request << endl;
    if (!device.SendData((char *)request.c_str(), request.length()))
    {
        return false;
    }

    uint32_t size = 0;
    string recv;

    // Parse the header and then the body.
    while (1)
    {
        size = device.RecvData(buffer, blockSize);
        if (size > 0)
        {
            recv = string(buffer, size);
            std::cout << "Received: " << recv << endl;
            break;
        }
    }

    responseHeaderTokens = TokenizeInput(recv);

    return true;
}

/*
SaveResponseBodyAsFile

Takes in parameters from header and saves the response body to a file
*/
bool SaveResponseBody(CTBDevice &device,
                      SaveResponseOpt &opt)
{
    char buffer[PACKET_SIZE];

    std::ofstream out;
    uint64_t total = 0;
    bool write = false;

    if (opt.saveFile)
    {
        write = true;
        out.open(opt.prefix + opt.filename);
        if (!out.is_open())
        {

            cerr << "SaveResponse : Could not open file for put." << endl;
            write = false;
        }
    }

    unsigned int loopCnt = 0;
    auto timer = chrono::steady_clock::now();
    while (total < opt.size)
    {
        uint32_t recvSize = device.RecvData(buffer, PACKET_SIZE);
        if (recvSize > 0)
        {
            if (write && opt.saveFile)
            {
                out.write(buffer, recvSize);
            }
            else
            {
                opt.result += string(buffer, recvSize);
            }

            total += recvSize;
            loopCnt++;
            if (loopCnt % 200 == 0 && opt.printDebug)
            {
                std::cout << "Transfer " << total << "/" << opt.size << endl;
            }
        }
    }
    auto msec = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - timer).count();
    std::cout << "Transfer complete " << total << "/" << opt.size << endl;
    std::cout << "Transfer took " << msec << " milliseconds." << endl;

    return true;
}

/*
SendFile

Sends file (includes put header and file sending), no need to send request beforehand.
*/
bool SendFile(CTBDevice &device, string prefix, string filepath,std::vector<string> &responseHeaderTokens)
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

    // Parse the header and then the body.
    char respBuffer[PACKET_SIZE*2];
    string recv;
    while (1)
    {
        uint32_t resSize = device.RecvData(respBuffer,PACKET_SIZE*2);
        if (resSize > 0)
        {
            recv = string(respBuffer, resSize);
            break;
        }
    }

    responseHeaderTokens = TokenizeInput(recv);

    return true;
}