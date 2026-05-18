#include "Client.hpp"

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

void Client::setRealName(std::string realname) {
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

    // 2. IRC rule: Nickname cannot start with a digit or a hyphen
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
            return &_clients[i]; // Return a pointer to the existing client
        }
    }
    return NULL; // Return NULL if no user with that nickname is found
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


void Client::setBuffer(std::string buffer)
{
    _buffer = buffer;
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
                std::cout << _clients[i].getBuffer() << " fd = " << Fd << std::endl;
                _clients[i].setBuffer("");
            }
            _clients[i].setBuffer(_clients[i].getBuffer() + Text);
            break;
        }
    }

}

