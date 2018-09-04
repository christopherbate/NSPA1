#include <iostream>
#include "../inc/SocketDevice.h"
#include "CTBDevice.h"

using namespace std;

int main()
{
    cout << "Testing... " << endl;
    int failed = 0;

    // 1 - test conn socket creation google.com:80
    SocketDevice Socket;
    SocketDevice Socket2;

    if (Socket.CreateSocket("www.google.com", "80") == false)
    {
        cerr << "Failed 1" << endl;
        failed++;
    }
    Socket.CloseSocket();

    cout << endl;

    // 2 - bad host
    if (Socket.CreateSocket("asdfdaf", "80") == true)
    {
        cerr << "Failed 1" << endl;
        failed++;
    }
    Socket.CloseSocket();

    cout << endl;

    // 3 - socket already open.
    Socket.CreateSocket("www.google.com", "80");
    if (Socket.CreateSocket("www.google.com", "80") == true)
    {
        cerr << "Failed 3" << endl;
        failed++;
    }
    Socket.CloseSocket();

    cout << endl;

    // 4 listen socket on 8080
    if (Socket.CreateSocket("8080") == false)
    {
        cerr << "Failed to create list socket." << endl;
        failed++;
    }
    Socket.CloseSocket();

    cout << endl;

    // 5 Try to listen twice (should fail)
    if (Socket.CreateSocket("8080") == false)
    {
        failed++;
    }
    else
    {
        if (Socket.CreateSocket("8080") == true)
        {
            cerr << "Allowed to create too many listen binds." << endl;
            failed++;
        }
    }
    Socket.CloseSocket();

    cout << endl;

    // 6 Try to send data.
    if (Socket.CreateSocket("8080"))
    {
        if (Socket2.CreateSocket("localhost", "8080"))
        {
            string msg = "stuff";
            if (Socket2.BlockingSend((char *)msg.c_str(), msg.length()) != msg.length())
            {
                cerr << "Short write." << endl;
                failed++;
            }
            else
            {
                cout << "Send successful" << endl;
            }
        }
        else
        {
            cerr << "6- failed to create socket 2" << endl;
            failed++;
        }
    }
    else
    {
        cerr << " 6 failed to create server socket" << endl;
        failed++;
    }
    cout << endl;
    Socket.CloseSocket();
    Socket2.CloseSocket();

    // 7 Try to send and recv data.
    if (Socket.CreateSocket("8080"))
    {
        if (Socket2.CreateSocket("localhost", "8080"))
        {
            string msg = "stuff";
            if (Socket2.BlockingSend((char *)msg.c_str(), msg.length()) != msg.length())
            {
                cerr << "Short write." << endl;
                failed++;
            }
            else
            {
                cout << "Send successful" << endl;
                char buffer[MAX_BUF_UDP];
                string address;
                unsigned int len =Socket.BlockingRecv(buffer,MAX_BUF_UDP,address);
                buffer[len] = '\0';
                cout <<"Addr:"<<address<<endl;
                cout <<"Len:"<<len<<endl;
                cout <<"Recv: "<<buffer<<endl;    
                if(Socket.DidTimeout()){
                    cerr << "7 - Timeout flag should not be set."<<endl;
                    failed++;
                }
            }
        }
        else
        {
            cerr << "6- failed to create socket 2" << endl;
            failed++;
        }
    }
    else
    {
        cerr << " 6 failed to create server socket" << endl;
        failed++;
    }
    cout << endl;
    Socket.CloseSocket();
    Socket2.CloseSocket();

    cout << endl;

    //8 - create ctb device
    CTBDevice client;
    CTBDevice server;
    if(!client.CreateDevice("www.google.com","80")){
        cerr << "Failed 8"<<endl;
        failed++;
    }
    client.DestroyDevice();

    // 9 - create client ands server
    if(server.CreateDevice("8080")){
        if(client.CreateDevice("localhost","8080")){
            cout << "All devices created."<<endl;
        } else {
            cerr << "9 - failed to create client."<<endl;
            failed++;
        }        
    } else {
        cerr << " 9 - failed to create server"<<endl;
        failed++;
    }
    client.DestroyDevice();
    server.DestroyDevice();

    cout<<endl;

    // 10 - set socket timeout.
    Socket.CreateSocket("8080");
    if(Socket.SetRecvTimeout(1000)==false){
        cerr << "10 - fail to set server timeout."<<endl;
        failed++;
    }
    Socket.CloseSocket();

    cout<<endl;

    // 11 - test timeout.
    Socket.CreateSocket("www.google.com","80");
    if(!Socket.SetRecvTimeout(1000000)){
        cerr << "Failed to set timeout"<<endl;
        failed++;
    }
    char dummy[100];
    string sdummy;
    if(Socket.BlockingRecv(dummy,100,sdummy,NULL)!=0){
        failed++;
        cerr << "Return should be 0 on timeout"<<endl;              
    } else {
        if(!Socket.DidTimeout()){
            failed++;
            cerr << "Timeout flag not set."<<endl;
        }
    }

    // final results.
    int total = 11;
    cerr << (total - failed) << "/" << total << " tests passed." << endl;
}