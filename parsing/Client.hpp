#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "parser.hpp"


class Client{

private:
    int _fd;
    std::string _buffer;
    std::string _nickName;
    std::string _userName;
    bool _isAuthenticated;
    std::string _realName;
    bool _isRegistered;        // This tracks if PASS + NICK + USER are done
public:
    Client();
    void setNickName(std::string nickname);
    void setUserName(std::string username);
    std::string getNickName() const;
    std::string getUserName() const;
    bool getIsAuthenticated() const;
    void setPassedPassword(bool isregistered);
    void setFd(int fd);
    int getFd() const;
    void setIsRegistered(bool status);
    bool getIsRegistered() const;
    
    void setRealName(std::string realname);
    std::string getRealName() const;
    bool checkIsValidNickname(const std::string& nick);
};

class Server{
private:
    std::string _password;
    std::string _port;
    std::vector<Client> _clients;
public:
    void setPassword(std::string pass);
    void setPort(std::string port);
    std::string getPassword() const;
    bool isNickInUse(const std::string& nick);
    // bool isNickInUse(const std::string& nick, int excludeFd);
    Client* getClientByNick(const std::string& nick);
    void addClient(const Client& client);
};

#endif
