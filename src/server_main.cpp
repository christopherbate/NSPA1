#include "CTBDevice.h"
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
    CTBDevice server;

    server.CreateDevice("8080");

    char buffer[MAX_BUF_UDP];
    unsigned long size = MAX_BUF_UDP;

    while(1){
        server.RecvData(buffer,size);
    }

    return 0;
}