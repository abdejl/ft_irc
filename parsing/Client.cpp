#include "Client.hpp"
#include "../channel/channel.hpp"
#include <sys/socket.h>

Client::Client() : _isAuthenticated(false), _isRegistered(false)
{

}
void Client::setNickName(std::string nickname)
{
   this->_nickName = nickname;  
}

void Client::setUserName(std::string username)
{
    this->_userName = username;
}

std::string Client::getNickName() const
{
    return(this->_nickName);
}

std::string Client::getUserName() const
{
    return(this->_userName);
}

void Server::setPassword(std::string pass)
{
    this->_password = pass;
}

void Server::setPort(std::string port)
{
    this->_port = port;
}

std::string Server::getPort() const
{
    return _port;
}

std::string Server::getPassword() const
{
    return(this->_password);
}

bool Client::getIsAuthenticated() const
{
    return (this->_isAuthenticated);
}

void Client::setPassedPassword(bool status)
{
    this->_isAuthenticated = status;  
}

void Client::setIsRegistered(bool status)
{
    this->_isRegistered = status;
}

bool Client::getIsRegistered() const
{
    return this->_isRegistered;
}

void Client::setRealName(std::string realname) 
{
    this->_realName = realname;
}
void Client::setFd(int fd)
{
    this->_fd = fd;
}

int Client::getFd() const
{
    return(this->_fd);
}

bool Client::checkIsValidNickname(const std::string& nick)
{
    std::string forbidden = " !@#$%^&*()+-";
    if (nick.empty())
        return false;
    if (nick.find_first_of(forbidden) != std::string::npos)
    {
        return false;
    }
    if (isdigit(nick[0]) || nick[0] == '-')
    {
        return false;
    }

    return true;
}

bool Server::isNickInUse(const std::string& nick)
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i].getNickName() == nick)
        {
            return true;
        }
    }
    return false;
}

Client* Server::getClientByNick(const std::string& nick)
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i].getNickName() == nick)
        {
            return &_clients[i];
        }
    }
    return NULL;
}

void Server::addClient(const Client& client)
{
    _clients.push_back(client);
}
std::string Client::getRealName() const
{
    return(this->_realName);
}

std::string Client::getBuffer() const
{
    return _buffer;
}


std::string Server::getBuffer(int i) const
{
    return _clients[i].getBuffer();
}

std::string& Client::getBufferRef()
{
    return _buffer;
}

void Client::setBuffer(std::string buffer)
{
    _buffer = buffer;
}

std::vector<Client>& Server::getClients()
{
    return _clients;
}

void    Server::FillClient(int Fd, std::string Text)
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i].getFd() == Fd)
        {
            if (_clients[i].getBuffer().find("\n") != std::string::npos
                || _clients[i].getBuffer().find("\r\n") != std::string::npos)
            {
                _clients[i].setBuffer("");
            }
            _clients[i].setBuffer(_clients[i].getBuffer() + Text);
            break;
        }
    }
}

void Server::processChannelJoin(Client &client, std::string channelName, std::string key)
{
    Channel *chan = getOrCreateChannel(channelName);
    if (!chan)
        return;
    if (!chan->canJoin(&client, key))
    {
        std::string err = "475 " + client.getNickName() + " " + channelName + " :Cannot join channel (+k or +l)\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (chan->hasClient(&client))
        return;
    chan->addClient(&client);
    if (chan->isEmpty())
        chan->addOperator(&client);
    std::string joinMsg = ":" + client.getNickName() + " JOIN " + channelName + "\r\n";
    send(client.getFd(), joinMsg.c_str(), joinMsg.length(), 0);
    chan->broadcast(joinMsg, &client);
}
