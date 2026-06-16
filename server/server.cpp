#include "server.hpp"

CoreServer::CoreServer(std::string Port)
{
    this->_Port = Port;
    _LenOfSocket = sizeof(sockaddr);// check is sockaddr or obj
    _BackLog = 10;
    _Event.fd = _ServerFd;
    _Event.events = POLLIN;
    _Event.revents = 0;
    Events.push_back(_Event);
}

int CoreServer::_ErrorDisplay(std::string Error)
{
    std::cerr << Error;
    return -1;
}

void    CoreServer::_Fill(sockaddr_in &Addr)
{
    Addr.sin_addr.s_addr = INADDR_ANY;
    Addr.sin_family = AF_INET;
    Addr.sin_port = htons(atoi(_Port.c_str()));
    // make the port able
}

int    CoreServer::PrepareServerSocket()
{
    sockaddr_in Addr;
    
    if ((_ServerFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return _ErrorDisplay("Error: socket() failed\n");
    Events[0].fd = _ServerFd;
    _Fill(Addr);
    if (bind(_ServerFd, (sockaddr *)&Addr, _LenOfSocket) == -1)
        return _ErrorDisplay("Error: bind() failed\n");
    if (listen(_ServerFd, _BackLog) == -1)
        return _ErrorDisplay("Error: bind() failed\n");
    std::cout << "Prepare done()\n";
    return 1;
}

void    CoreServer::_AddClient(Server &server)
{
    Client  C;

    C.setFd(_Event.fd);
    C.setBuffer("");
    _Event.events = POLLIN;
    _Event.revents = 0;
    Events.push_back(_Event);
    server.addClient(C);
}

int CoreServer::Receive(Server &server)
{
    char    buffer[1024];
    int     bytes_read;

    memset(buffer, 0, sizeof(buffer));
    if (poll(&Events[0], Events.size(), -1) == -1)
        _ErrorDisplay("Error: poll() failed\n");
    if (Events[0].revents & POLLIN)
    {
        Events[0].revents = 0;
        if ((_Event.fd = accept(_ServerFd, (sockaddr *)&_ClientAddr, &_LenOfSocket)) == -1)
        {
            _ErrorDisplay("Errro: accept() failed\n");
            return -1;
        }
        _AddClient(server);//if anything wrong check _Event.fd in condition
        return 1;
    }
    for (size_t i = 1; i < Events.size(); i++)
    {
        if (Events[i].revents & POLLIN)
        {
            if ((bytes_read = recv(Events[i].fd, &buffer, 1023, 0)) == -1)
            {
                _ErrorDisplay("Error: recv() failed\n");
                return -1;
            }
            if (!bytes_read)
            {
                std::cout << "ctrl+C\n";
                close(Events[i].fd);
                Events.erase(Events.begin() + i);
                continue;
            }
            server.FillClient(Events[i].fd, buffer);
        }
        if (Events[i].revents & POLLHUP)
        {
            std::cout << "PULLHUP\n";
            close(Events[i].fd);
            Events.erase(Events.begin() + i);
        }
        Events[i].revents = 0;
    }
    return 1;
}

