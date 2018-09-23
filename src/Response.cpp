#include "Response.h"

/*
ParseRequest
*/
bool ParseRequest(CTBDevice &device, const char *request, uint32_t length,
                  RequestedDetails &details)
{
    string input = string(request, length);
    std::vector<string> tokens = TokenizeInput(input);
    CmdType cmd = ParseCmd(tokens[0]);
    switch (cmd)
    {
    case GET:
        details.type = GET;
        details.filename = tokens[1];
        break;
    case PUT:
        details.type = PUT;
        details.filename = tokens[1];
        details.bodySize = stoull(tokens[2]);
        break;
    case DELETE:
        details.type = DELETE;
        details.filename = tokens[1];
        break;
    case LS:
        details.type = LS;
        break;
    default:
        details.type = UNK;
        break;
    }

    return true;
}

/*
bool Send response header and body

Sends a response with the given options. Body can be msg or file.
*/
bool SendResponse(CTBDevice &device, ResponseOpt &opt)
{
    const uint32_t blockSize = PACKET_SIZE / 2;
    char buffer[blockSize];
    uint64_t bodySize;
    ifstream infile;

    // If this is a file, generate the header.
    // Otherwise, the header is provided.
    if (opt.isFile)
    {
        // Open the file for reading.
        infile.open(opt.prefix + opt.filename);
        if (!infile.is_open())
        {
            cerr << "File does not exist in files directory." << endl;
            opt.header = "none 0";
        }
        else
        {
            // Get the size of the file.
            infile.seekg(0, ios::end);
            bodySize = infile.tellg();
            infile.seekg(0, ios::beg);
            opt.header = opt.filename + " " + std::to_string(bodySize);
        }
    }

    // SEND header
    cout << "Sending : " << opt.header << endl;
    auto timer = chrono::steady_clock::now();
    device.SendData((char *)opt.header.c_str(), opt.header.length());

    // SEND body.
    uint64_t total = 0;
    if (opt.isFile && opt.header != "none 0")
    {
        while (!infile.eof())
        {
            infile.read(buffer, blockSize);
            if (infile.gcount() > 0)
            {
                device.SendData(buffer, infile.gcount());
                total += infile.gcount();
            }
        }
    } else if (opt.msg.length() >= 0)
    {
        uint32_t currPos = 0;
        while(currPos < opt.msg.length()){
            uint32_t lengthToSend = opt.msg.length() - currPos;
            if(lengthToSend > blockSize)
                lengthToSend = blockSize;
            device.SendData(opt.msg.c_str()+currPos, lengthToSend);
            currPos+=lengthToSend;
        }
    }

    // Stats.
    auto msec = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - timer).count();
    cout << "Transfer complete " << total << "/" << bodySize << endl;
    cout << "Transfer took " << msec << " milliseconds." << endl;

    return true;
}

bool RecvFile(CTBDevice &device, string prefix, string filename, uint64_t size, bool print)
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


string ReadLS()
{
    string cmd = "/bin/ls -l";
    string res = "";
    if (ExecuteCMD(cmd, res))
    {
        return res;
    }

    return "";
}
