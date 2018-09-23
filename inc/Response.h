#ifndef RESPONSE_H
#define RESPONSE_H

#include "CTBDevice.h"
#include "Shared.h"

struct RequestedDetails
{
    CmdType type;
    string filename;
    uint64_t bodySize;
};

struct ResponseOpt
{
    bool isFile = false;
    string msg = "";
    string prefix = "";
    string header = "ok";
    string filename = "";
};

/*
ParseRequest
*/
bool ParseRequest(CTBDevice &device, const char *request, uint32_t length, RequestedDetails &details);

/*
bool Send response Body

Sends a response with the given options
*/
bool SendResponse(CTBDevice &device, ResponseOpt &opt);

bool RecvFile(CTBDevice &device,
              string prefix,
              string filename,
              uint64_t size,
              bool print = false);

string ReadLS();

#endif