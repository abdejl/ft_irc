#include "channel.hpp"
#include <sys/socket.h> // For send()


Channel::Channel()
{
    _topic = "";
    _key = "";
    _inviteOnly = false;
    _topicRestricted = false;
    _userLimit = -1;
}

std::string Channel::getName()
{
    return _name;
}

bool Channel::isEmpty()
{
    return (_vClient.empty());
}

bool Channel::isOperator(Client *client)
{
    for (std::vector<Client*>::iterator it = _vOperator.begin(); it != _vOperator.end(); it++)
    {
        if ((*it)->getFd() == client->getFd())
            return true;
    }
    return false;
}

void Channel::addClient(Client *client)
{
    _vClient.push_back(client);
}

void Channel::removeClient(Client *client, bool printMSG)
{
    if (isEmpty())
        return;
    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
    {
        if (*it == client)
        {
            if (printMSG)
                broadcast(client->getNickName() + "PART" + _name, client);
            _vClient.erase(it);
            break;
        }
    }
}

void Channel::broadcast(std::string message, Client *sender)
{
    std::string newMessage = ":" + sender->getNickName() + " PRIVMSG " + _name + " :" + message;

    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
    {
        if ((*it)->getFd() != sender->getFd())
        { 
            send((*it)->getFd(), newMessage.c_str(), newMessage.length(), 0);
            // (*it)->send(newMessage);
        }
    }
}

void Channel::addOperator(Client *client)
{
    std::cout << "TEST ADDOPERATOR" << std::endl;
    _vOperator.push_back(client); 
}

void Channel::removeOperator(Client *client)
{
    for (std::vector<Client*>::iterator it = _vOperator.begin(); it != _vOperator.end(); it++)
    {
        if (*it == client)
        {
            _vOperator.erase(it);  
            break;
        }
    }
}

void Channel::setTopic(std::string topic)
{
    _topic = topic;
}

std::string Channel::getTopic()
{
    return _topic;
}

void Channel::setTopicRestricted(bool value)
{
    _topicRestricted = value;
}

bool Channel::isTopicRestricted()
{
    return _topicRestricted;
}

bool Channel::isInviteOnly()
{
    return _inviteOnly;
}

void Channel::setInviteOnly(bool value)
{
    _inviteOnly = value;
}

std::string Channel::getKey()
{
    return _key;
}

void Channel::setKey(std::string key)
{
    _key = key;
}

bool Channel::hasKey()
{
    return !_key.empty();
}

int Channel::getUserLimit()
{
    return _userLimit;
}

void Channel::setUserLimit(int l)
{
    _userLimit = l;
}

bool Channel::hasLimit()
{
    return _userLimit != -1;
}

void handleMode(Channel &channel, std::string mode, std::string arg)
{
    if (mode == "+i")
        channel.setInviteOnly(true);
    else if (mode == "-i")
        channel.setInviteOnly(false);
    else if (mode == "+k")
        channel.setKey(arg);
    else if (mode == "-k")
        channel.setKey("");
    else if (mode == "+l")
        channel.setUserLimit(std::atoi(arg.c_str()));
    else if (mode == "-l")
        channel.setUserLimit(-1);
}

bool Channel::canJoin(Client *client, std::string key)
{
    if (hasLimit() && _vClient.size() >= (size_t)_userLimit)
        return false;
    if (hasKey() && _key != key)
        return false;
    if (isInviteOnly() && !isInvited(client))
        return false;
    return true;
}

void Channel::inviteClient(Client *client)
{
    _vInvitedClients.push_back(client);
}

bool Channel::isInvited(Client *client)
{
    for (std::vector<Client*>::iterator it = _vInvitedClients.begin(); it != _vInvitedClients.end(); it++)
    {
        if (client == *it)
            return true;
    }
    return false;
}

void Channel::removeInvitation(Client *client)
{
    for (std::vector<Client*>::iterator it = _vInvitedClients.begin(); it != _vInvitedClients.end(); it++) 
    {
        if (*it == client)
        {
            _vInvitedClients.erase(it);
            return;
        }
    }
}

void Channel::kickClient(Channel &channel, Client *sender, Client *target)
{
    if (!channel.isOperator(sender))
    {
        std::string msg = "482 #" + channel.getName() + " :You're not channel operator";
        send(sender->getFd(), msg.c_str(), msg.length(), 0);
        // sender->send("481 #" + channel.getName() + " :You're not channel operator");
        return;
    }
    channel.broadcast(":" + sender->getNickName() + " KICK #" + channel._name + " " + target->getNickName(), sender);
    removeClient(target, false);
    removeOperator(target);
}

bool Channel::isRestrictedTopic()
{
    return _topicRestricted;
}

void Channel::ChangeTopic(Channel &channel, Client *sender, std::string topic)
{
    if (isRestrictedTopic() && !isOperator(sender))
    {
        std::string msg = "482 #" + channel.getName() + " :You're not channel operator";
        // sender->send("482 #" + channel.getName() + " :You're not channel operator");
        send(sender->getFd(), msg.c_str(), msg.length(), 0);
    }
    channel.broadcast(":" + sender->getNickName() + " TOPIC #" + channel._name + " :" + topic,
                  sender);
    channel.setTopic(topic); 
}

void Channel::inviteToChannel(Channel &channel,Client *sender, Client *target)
{
    if (!channel.isOperator(sender))
    {
        std::string msg = "482 #" + channel.getName() + " : You're not channel operator";
        // sender->send("482 #" + channel.getName() + " :You're not channel operator");
        send(sender->getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    channel.inviteClient(target);

    std::string msg = ":" + sender->getNickName()+ " INVITE "+ target->getNickName()+ " #" + channel._name;
    send(sender->getFd(), msg.c_str(), msg.length(), 0);
    // target->send(":" + sender->getNickName()+ " INVITE "+ target->getNickName()+ " #" + channel._name); 
}

void Join(Channel &channel, Client *client)
{
    if (channel.isEmpty())
        channel.addOperator(client);
}

bool Channel::hasClient(Client *client)
{
    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
    {
        if (*it == client)
            return true;
    }
    return false;
}

std::vector<std::string> Channel::getClientList()
{
    std::vector<std::string> clientList;
    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
    {
        clientList.push_back((*it)->getNickName());
    }
    return clientList;
}

bool Channel::hasDuplicateNickName(std::string nickname)
{
    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
    {
        if ((*it)->getNickName() == nickname)
            return true;
    }
    return false;
}


Channel* Server::getChannelByName(const std::string& name)
{
    (void)name;
    //for testing only 
    //sawb dialk
    return NULL;
}
