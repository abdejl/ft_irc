#include "Client.hpp"


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

std::string Server::getPassword() const
{
    return(this->_password);
}

bool Client::getIsAuthenticated() const
{
   if(this->_isAuthenticated) 
        return (true);
    return(false);
}

void Client::setPassedPassword(bool isregistered)
{
    this->_isAuthenticated = true;  
}
