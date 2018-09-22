#include "SocketDevice.h"

/*
Constructor
*/
SocketDevice::SocketDevice()
{
    m_fd = -1;
    m_type = PASSIVE;
    m_remoteHost = "";
    m_remotePort = "";
}

/*
Desctructor
*/
SocketDevice::~SocketDevice()
{
    if (m_fd != -1)
    {
        cout << "SocketDevice : Closing socket." << endl;
        close(m_fd);
    }
}

/*
CloseSocket
Closes socket owned by class, if one exists.
*/
void SocketDevice::CloseSocket()
{
    if (m_fd != -1)
        close(m_fd);
    m_fd = -1;
}

/*
CreateSocket (connecting)
Creates a connecting (client) socket and attempts to connect to specified host and port
returns true if connection was successful, false otherwise.
*/
bool SocketDevice::CreateSocket(string host, string port)
{
    if (m_fd != -1)
    {
        cerr << "Socket already open at fd: " << m_fd << ". Close before reusing this interface." << endl;
        return false;
    }
    m_type = CONN;
    // family - AF_INET (ip)
    // socktype - SOCK_DGRAM (UDP)
    // protocol - IPPROTO_UDP
    // flags -
    /*  
            Unix manpage:
            If the AI_PASSIVE flag is not set in hints.ai_flags, then the
            returned socket addresses will be suitable for use with connect(2),
            sendto(2), or sendmsg(2).  If node is NULL, then the network address
            will be set to the loopback interface address (INADDR_LOOPBACK for
            IPv4 addresses, IN6ADDR_LOOPBACK_INIT for IPv6 address); this is used
            by applications that intend to communicate with peers running on the
            same host.
        */
    // flags - AI_NUMERICSERV
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_NUMERICSERV | AI_CANONNAME;

    // Get the address info structure
    struct addrinfo *resList;
    int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &resList);
    if (rc != 0)
    {
        cerr << "SocketDevice : CreateSocket : getaddrinfo : " << rc << endl;
        return false;
    }

    // Check first for empty address.
    if (resList == NULL)
    {
        cerr << "SocketDevice : CreateSocket : no addresses at hostname/port" << endl;
        return false;
    }

    // Parse the list and try to connect.
    struct addrinfo *aiIter = resList;
    while (aiIter != NULL)
    {
        char nameBuffer[INET_ADDRSTRLEN];
        inet_ntop(aiIter->ai_family, aiIter->ai_addr, nameBuffer, INET_ADDRSTRLEN);
        cout << "Attempting to create UDP socket with cached target: " << (nameBuffer != NULL ? nameBuffer : "") << " " << resList->ai_canonname << endl;
        m_fd = socket(aiIter->ai_family, aiIter->ai_socktype, aiIter->ai_protocol);

        if (m_fd == -1)
        {
            cerr << "SocketDevice : CreateSocket : socket; trying next address " << endl;
            aiIter = aiIter->ai_next;
            continue;
        }

        if (connect(m_fd, aiIter->ai_addr, aiIter->ai_addrlen) == -1)
        {
            cerr << "SocketDevice : CreateSocket : connect; errno " << errno << endl;
            aiIter = aiIter->ai_next;
            close(m_fd);
            continue;
        }
        else
        {
            // Since we called connect as a DATAGRAM, these paramters are already saved.
            // But if needed to use sendto, we could use these saved parameters.
            m_remoteHost = host;
            m_remotePort = port;
            memcpy(&m_remoteAddr, aiIter->ai_addr, sizeof(*aiIter->ai_addr));
        }
        break;
    }

    // Free address info list
    freeaddrinfo(resList);

    if (aiIter == NULL)
    {
        cerr << "SocketDevice : failed to connect to any addresses." << endl;
        close(m_fd);
        m_fd = -1;
        return false;
    }

    cout << "UDP Socket established." << endl;

    return true;
}

/*
CreateSocket (passive/listening)
Creates a listening socket on specified port.
Returns true if bind was successful, false otherwise
*/
bool SocketDevice::CreateSocket(string port)
{
    if (m_fd != -1)
    {
        cerr << "Socket already open at fd: " << m_fd << ". Close before reusing this interface." << endl;
        return false;
    }
    m_type = PASSIVE;
    // family - AF_INET (ip)
    // socktype - SOCK_DGRAM (UDP)
    // protocol -
    // flags -
    /*  
            Unix manpage:
                If the AI_PASSIVE flag is specified in hints.ai_flags, and node is
            NULL, then the returned socket addresses will be suitable for
            bind(2)ing a socket that will accept(2) connections.  The returned
            socket address will contain the "wildcard address" (INADDR_ANY for
            IPv4 addresses, IN6ADDR_ANY_INIT for IPv6 address).  The wildcard
            address is used by applications (typically servers) that intend to
            accept connections on any of the host's network addresses.  If node
            is not NULL, then the AI_PASSIVE flag is ignored.
        */
    // hints - AI_NUMERICSERV and AI_PASSIVE
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;

    // get the address info structure
    struct addrinfo *resList;
    int rc = getaddrinfo(NULL, port.c_str(), &hints, &resList);
    if (rc != 0)
    {
        cerr << "SocketDevice : CreateSocket : getaddrinfo : " << rc << endl;
        cerr << "Err: " << strerror(errno) << endl;
        return false;
    }

    // Loop through and create socket
    struct addrinfo *aiIter = resList;
    while (aiIter != NULL)
    {

        char nameBuffer[INET_ADDRSTRLEN];
        inet_ntop(aiIter->ai_family, aiIter->ai_addr, nameBuffer, INET_ADDRSTRLEN);
        cout << "Attempting to create socket for: " << (nameBuffer != NULL ? nameBuffer : "") << endl;

        // Create the socket.
        m_fd = socket(aiIter->ai_family, aiIter->ai_socktype, aiIter->ai_protocol);
        if (m_fd == -1)
        {
            cerr << "SocketDevice : CreateSocket : socket" << endl;
            aiIter = aiIter->ai_next;
            continue;
        }

        if (bind(m_fd, aiIter->ai_addr, aiIter->ai_addrlen) == -1)
        {
            cerr << "SocketDevice : CreateSocket : bind; errno " << strerror(errno) << endl;
            aiIter = aiIter->ai_next;
            close(m_fd);
            continue;
        }
        break;
    }

    // Free address info list
    freeaddrinfo(resList);

    if (aiIter == NULL)
    {
        cerr << "SocketDevice : failed to bind to any addresses." << endl;
        close(m_fd);
        m_fd = -1;
        return false;
    }

    cout << "UDP Socket successfully bound to port " << port << endl;

    return true;
}

/*
SetRecvTimeout

Sets the amount of time the socket will wait on recv until returning if no data

*/
bool SocketDevice::SetRecvTimeout(uint64_t usec)
{
    if (m_fd == -1)
        return false;
    struct timeval to;
    to.tv_usec = usec % 1000000;
    to.tv_sec = (usec >= 1000000 ? (usec - to.tv_usec) / 1000000 : 0);
    int rc = setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(struct timeval));
    if (rc != 0)
    {
        cerr << "SocketDevice : SetRecvTimeout : setsockopt, errno " << errno << endl;
        return false;
    }

    return true;
}

/*
BlockingSend

Sends data at at data of size length to remoteAddr (if specified, otherwise to already save remote
*/
uint32_t SocketDevice::BlockingSend(const char *data, unsigned long length, struct sockaddr *remoteAddr)
{
    if (m_fd < 0)
    {
        cerr << "Socket not open." << endl;
        return 0;
    }
    if (m_type == PASSIVE && remoteAddr == NULL)
    {
        remoteAddr = (struct sockaddr *)&m_remoteAddr;
    }

    //cout << "Sending: " << data << " of length " << length << endl;
    ssize_t sl = length;
    ssize_t res = 0;

    // If we are passive, we need to specify address. Otherwise, we saved it using "connect"
    // although both are using datagram udp.
    if (m_type == PASSIVE)
        res = sendto(m_fd, data, sl, 0, remoteAddr, sizeof(struct sockaddr_storage));
    else
        res = send(m_fd, data, sl, 0);

    if (res == -1)
    {
        cerr << "Blocking send : error " << strerror(errno) << endl;
        return 0;
    }

    if (res != sl)
    {
        cerr << "Failed/short send: " << res << "/" << length << endl;
        return res;
    }
    return res;
}

/*
    BlockingRecv

    Receives data into buffer of at most size 

    Saves receiving address into remoteAddr if not null

    Receive from recvAddress 
*/
uint32_t SocketDevice::BlockingRecv(char *buffer, unsigned long size, string &recvAddress, struct sockaddr *remoteAddr)
{
    if (m_fd < 0)
    {
        cerr << "Socket not open." << endl;
        return 0;
    }
    m_timeout = false;
    struct sockaddr_storage recvAddr;
    socklen_t recvAddrLen = sizeof(struct sockaddr_storage);
    ssize_t res = recvfrom(m_fd, buffer, size, 0, (struct sockaddr *)&recvAddr, &recvAddrLen);

    if (res == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            //cerr << "Timeout" << endl;
            m_timeout = true;
        }
        return 0;
    }
    char nameBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &recvAddr, nameBuffer, INET_ADDRSTRLEN);
    recvAddress = string(nameBuffer);

    if (remoteAddr)
    {
        memcpy(remoteAddr, &recvAddr, recvAddrLen);
    }
    memcpy(&m_remoteAddr, &recvAddr, recvAddrLen);

    return res;
}