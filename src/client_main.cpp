#include "CTBDevice.h"
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
    CTBDevice client;

    client.CreateDevice("localhost","8080");

    string msg = "yoyoyo";
    client.SendData((char*)msg.c_str(),msg.length());

    return 0;
}