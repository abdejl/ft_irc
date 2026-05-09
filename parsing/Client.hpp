#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "parser.hpp"


class Client{

private:
    int _fd;
    std::string _buffer;
    std::string _nickName;
    std::string _userName;
    bool _isAuthenticated = true;
public:
    void setNickName(std::string nickname);
    void setUserName(std::string username);
    std::string getNickName() const;
    std::string getUserName() const;
    bool getIsAuthenticated() const;
    void setPassedPassword(bool isregistered);
};

class Server{
private:
    std::string _password;
    std::string _port;
public:
    void setPassword(std::string pass);
    void setPort(std::string port);
    std::string getPassword() const;
};

#endif
