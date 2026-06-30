#ifndef SERVER_HPP
#define SERVER_HPP

#define SERVER_PORT 6667

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../parsing/Client.hpp"


class   CoreServer
{
private:
    int         _ServerFd;
    int         _BackLog;
    std::string _Port;
    socklen_t   _LenOfSocket;
    pollfd      _Event;
    sockaddr_in _ClientAddr;
    std::vector<pollfd> Events;

    int         _ErrorDisplay(std::string Error);
    void        _Fill(sockaddr_in &Addr);
    void        _AddClient(Server &server);
public:
                CoreServer(std::string Port);
    int         PrepareServerSocket();
    int         Receive(Server &server);
};


#endif
