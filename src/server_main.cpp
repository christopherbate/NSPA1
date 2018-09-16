#include "CTBDevice.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
using namespace std;

#define FILES_DIR "./"

#define BLOCK_SIZE 131072

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " [port]" << endl;
    }

    CTBDevice server;

    if(!server.CreateDevice(argv[1])){
        cerr <<"Error creating server."<<endl;
        return -1;
    }

    char buffer[BLOCK_SIZE];
    unsigned long size = BLOCK_SIZE;
    //string files_dir = FILES_DIR;

    while (1)
    {
        break;
    }

    return 0;
}