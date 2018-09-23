#ifndef REQUEST_H
#define REQUEST_H

#include "CTBDevice.h"

/*
    SaveResponseOpt 

    Pass this to SaveResponseBody in order to change how response is saved.
    Response will be put in "result" string UNLESS saveFile = true, in which
    the response will be saved to the file at prefix/filename

    SIZE parameter must ALWAYS be given.
*/
struct SaveResponseOpt{
    bool saveFile = true;
    string prefix = "./";
    string filename = "example.txt";
    string result = "";    
    uint64_t size = 0;
    bool printDebug = true;
};

/*
Request 

Helper Function
Sends a request given by "request" string using device.
Awaits response (blocking) and returns response header 
as vector of string tokens (space seperated).
*/
bool Request(CTBDevice &device, string request, std::vector<string> &responseHeaderTokens);

/*
SaveResponseBodyAsFile

Takes in parameters from header and saves the response body to a file
*/
bool SaveResponseBody(CTBDevice &device,
                      SaveResponseOpt &opt);

/*
SendFile

Sends file (includes put header and file sending), no need to send request beforehand.
*/
bool SendFile(CTBDevice &device, string prefix, string filepath, std::vector<string> &responseHeaderTokens);

#endif