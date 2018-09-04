#include "CTBDevice.h"

/*
constructor
*/
CTBDevice::CTBDevice()
{
    m_created = false;
    m_type = CLIENT;
}

/*
Destructor
*/
CTBDevice::~CTBDevice()
{
    // Always safe.
    m_socket.CloseSocket();
}

/*
DestroyDevice
*/
void CTBDevice::DestroyDevice()
{
    m_created = false;
    m_socket.CloseSocket();
}

/*
CreateDevice (client mode)
*/
bool CTBDevice::CreateDevice(string host, string port)
{
    if (m_created)
        return false;
    if (!m_socket.CreateSocket(host, port))
    {
        return false;
    }

    m_created = true;
    m_type = CLIENT;
    return true;
}

/*
CreateDevice (server mode)
*/
bool CTBDevice::CreateDevice(string port)
{
    if (m_created)
        return false;
    if (!m_socket.CreateSocket(port))
        return false;

    m_created = true;
    m_type = SERVER;
    return true;
}

/*
SendData
For reliably sending in-memory data.
*/
bool CTBDevice::SendData(char *data, unsigned long size, unsigned int maxRetry)
{
    if (!m_created)
        return false;

    string alert = "SEND-" + std::to_string(size);

    if (m_socket.BlockingSend((char *)alert.c_str(), alert.length()) != alert.length())
    {
        cerr << "CTBDevice : BlockingSend : incorrect send count " << endl;
        return false;
    }

    // Transmission loop
    char buff[MAX_BUF_UDP];
    string addr;
    char *lastMsg = (char*)alert.c_str();
    unsigned long lastLen = alert.length();
    unsigned long len;
    unsigned int retryCount = 0;
    if (!m_socket.SetRecvTimeout(1000000))
    {
        cerr << "Warning: Failed to set timeout." << endl;
    }
    while (1)
    {
        len = m_socket.BlockingRecv(buff, MAX_BUF_UDP, addr);

        // Retransmit our request if we are timedout.
        if (len == 0 && m_socket.DidTimeout())
        {
            // DEBUG
            cout << "Timeout exceeded, attempting retry "<< retryCount<<endl;
            if (retryCount < maxRetry)
            {
                retryCount++;
                if (m_socket.BlockingSend(lastMsg,lastLen) != lastLen )
                {
                    cerr << "CTBDevice : BlockingSend : incorrect send count " << endl;
                    return false;
                }
                continue;
            }
            else
            {
                // DEBUG
                cerr << "Retry count exceeded." << endl;
                return false;
            }            
        }

        buff[len] = '\0';
        string payload = string(buff);
        if (payload == "SEND")
        {
            cout << "Sending user-specified data." << endl;
            lastMsg = data;
            lastLen = size;
            len = m_socket.BlockingSend(data, size);
        }
        else if (payload == "OK")
        {
            cout << "Send successful" << endl;
            break;
        }
    }

    return true;
}

/*
RecvData
*/
bool CTBDevice::RecvData(char *data, unsigned long &size)
{
    // Check that we have been created.
    if (!m_created)
        return false;

    // TODO - change to vector of packets
    char buff[MAX_BUF_UDP];
    unsigned long len;
    struct sockaddr_storage recvAddr;
    string addr;
    string response; // response string
    bool expectRecv = false;
    char *lastMsg = NULL;
    unsigned long lastLen = 0;
    const unsigned int maxRetry = 0;
    unsigned int retryCount = 0;

    // Ensure we are not timing out.
    m_socket.SetRecvTimeout(0);

    // Recv packet on the socket.
    len = m_socket.BlockingRecv(buff, MAX_BUF_UDP, addr, (struct sockaddr *)&recvAddr);

    // DEBUG -terminate the buffer and print
    buff[len] = '\0';
    cout << "Msg " << buff << " from " << addr << endl;

    // Tokenize the incoming payload
    vector<string> payloadTokens;
    stringstream tokenStream(buff);
    string token;
    while (getline(tokenStream, token, '-'))
    {
        payloadTokens.push_back(token);
    }

    // Translate
    if (payloadTokens[0] == "SEND")
    {
        // DEBUG
        cout << "Incoming data." << endl;

        expectRecv = true;
        if (payloadTokens.size() != 2)
        {
            cerr << "RecvData : missing payload size parameter. Assuming default size." << endl;
            size = MAX_BUF_UDP;
        }
        else
        {
            size = stoul(payloadTokens[1]);
        }

        // Send theresponse
        response = "SEND";
        lastMsg = (char*)response.c_str();
        lastLen = response.length();
        m_socket.BlockingSend((char *)response.c_str(), response.length(), (struct sockaddr *)&recvAddr);
    }

    // Transmission loop.
    unsigned long total = 0;
    m_socket.SetRecvTimeout(1000000);
    while (expectRecv)
    {
        len = m_socket.BlockingRecv(buff, MAX_BUF_UDP, addr);

        // Handle timeout
        // Retransmit our request if we are timedout.
        if (len == 0 && m_socket.DidTimeout())
        {
            // DEBUG
            cout << "Timeout exceeded, attempting retry "<< retryCount <<endl;

            if (retryCount < maxRetry)
            {
                retryCount++;
                if (m_socket.BlockingSend(lastMsg,lastLen, (struct sockaddr *)&recvAddr) != lastLen )
                {
                    cerr << "CTBDevice : BlockingSend : incorrect send count " << endl;
                    return false;
                }
                continue;
            }
            else
            {
                // DEBUG
                cerr << "Retry count exceeded." << endl;
                return false;
            }            
        }

        total += len;
        buff[len] = '\0';
        cout << "Recv: " << buff << endl;
        cout << "Recv: " << total << "/" << len << " bytes." << endl;

        if (total == size)
        {
            response = "OK";
            m_socket.BlockingSend((char *)response.c_str(), response.length(), (struct sockaddr *)&recvAddr);
            break;
        }
    }

    cout << "Recv complete." << endl;

    return true;
}