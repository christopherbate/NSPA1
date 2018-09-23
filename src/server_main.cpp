#include "CTBDevice.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <fstream>
#include "Shared.h"
#include "Response.h"
using namespace std;

#define FILES_DIR "./"

void runServer(CTBDevice *server, string port, bool *cancel)
{
    if (server->Listen(port, *cancel))
    {
        while (!*cancel)
        {
            server->Update(false);
            //std::this_thread::sleep_for(chrono::milliseconds(10));
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " [port]" << endl;
        return -1;
    }

    CTBDevice server;
    bool cancel = false;

    if (!server.CreateDevice())
    {
        cerr << "Error creating server." << endl;
        return -1;
    }

    std::thread serverThread(runServer, &server, argv[1], &cancel);
    cout << "Server running on " << argv[1] << endl;
    char buffer[PACKET_SIZE*2];
    while (!cancel)
    {
        uint32_t size = server.RecvData(buffer, PACKET_SIZE*2);
        if (size > 0)
        {
            RequestedDetails req;
            ResponseOpt opt;
            if(ParseRequest(server, buffer,size,req)){
                if(req.type == GET){
                    opt.isFile = true;
                    opt.filename = req.filename;
                    opt.prefix = "./";                    
                    SendResponse(server,opt);
                } else if (req.type == PUT){
                    if(RecvFile(server,"./",req.filename,req.bodySize)){
                        opt.header = "ok";
                    } else {
                        opt.header = "fail";
                    }
                    opt.isFile = false;
                    opt.msg = "";                    
                    SendResponse(server,opt);
                } else if (req.type == LS){
                    string lsResult = ReadLS();
                    opt.isFile = false;
                    opt.msg = lsResult;
                    opt.header = "ok "+to_string(lsResult.length());
                    SendResponse(server,opt);
                } else if (req.type == DELETE){
                    string cmd = "rm ./"+req.filename;
                    string rmRes;
                    if(ExecuteCMD(cmd,rmRes)){
                        opt.isFile = false;                        
                        opt.header = "ok";
                        opt.msg = "";
                        SendResponse(server,opt);
                    } else {
                        opt.header = "fail";
                        opt.isFile = false;
                        opt.msg="";
                        SendResponse(server,opt);
                    }
                }
            } else {
                // Send reset
            }
        }
        else
        {
            std::this_thread::sleep_for(chrono::seconds(1));
        }
    }

    cout <<"Server exiting"<<endl;

    return 0;
}