#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "parser.hpp"
#include <map>
class Channel;
class Client{

private:
    int _fd;
    std::string _buffer;
    std::string _nickName;
    std::string _userName;
    bool _isAuthenticated;
    std::string _realName;
    bool _isRegistered;
public:
    Client();
    void setNickName(std::string nickname);
    void setUserName(std::string username);
    void setBuffer(std::string buffer);
    std::string getNickName() const;
    std::string getUserName() const;
    bool getIsAuthenticated() const;
    void setPassedPassword(bool isregistered);
    void setFd(int fd);
    int getFd() const;
    void setIsRegistered(bool status);
    bool getIsRegistered() const;
    std::string getBuffer() const;   
    void setRealName(std::string realname);
    std::string getRealName() const;
    bool checkIsValidNickname(const std::string& nick);
    std::string& getBufferRef();
    std::string getHostmask() const;
};

class Server{
private:
    std::string _password;
    std::string _port;
    std::vector<Client*> _clients;
    std::map<std::string, Channel*> _channels;

public:
    void setPassword(std::string pass);
    void setPort(std::string port);
    std::string getBuffer(int i) const;
    std::string getPort()const;
    std::string getPassword() const;
    bool isNickInUse(const std::string& nick);
    Client* getClientByNick(const std::string& nick);
    void    addClient(Client* client);
    void    removeClient(int fd);
    void    FillClient(int Fd, std::string Text);
    std::vector<Client*>& getClients();
    Channel* getChannelByName(const std::string& name);
    Channel* getOrCreateChannel(const std::string& name);
    void processChannelJoin(Client &client, std::string channelName, std::string key);
};

#endif