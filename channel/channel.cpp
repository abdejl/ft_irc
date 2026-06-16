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

void Channel::setName(std::string name)
{
    _name = name;
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
            {
                std::string partMsg = ":" + client->getNickName() + " PART " + _name + "\r\n";
                broadcast(partMsg, client);
            }
            _vClient.erase(it);
            break;
        }
    }
}

void Channel::broadcast(std::string message, Client *sender)
{
    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
    {
        if ((*it)->getFd() != sender->getFd())
        {
            send((*it)->getFd(), message.c_str(), message.length(), 0);
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
        std::string msg = "482 " + channel.getName() + " :You're not channel operator\r\n";
        send(sender->getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string kickMsg = ":" + sender->getNickName() + " KICK " + channel._name + " " + target->getNickName() + "\r\n";
    channel.broadcast(kickMsg, sender);
    channel.removeClient(target, false);
    channel.removeOperator(target);
}

bool Channel::isRestrictedTopic()
{
    return _topicRestricted;
}

void Channel::ChangeTopic(Channel &channel, Client *sender, std::string topic)
{
    if (isRestrictedTopic() && !isOperator(sender))
    {
        std::string msg = "482 " + channel.getName() + " :You're not channel operator\r\n";
        send(sender->getFd(), msg.c_str(), msg.length(), 0);
    }
    std::string topicMsg = ":" + sender->getNickName() + " TOPIC " + channel._name + " :" + topic + "\r\n";
    channel.broadcast(topicMsg, sender);
    channel.setTopic(topic);
}

void Channel::inviteToChannel(Channel &channel,Client *sender, Client *target)
{
    if (!channel.isOperator(sender))
    {
        std::string msg = "482 " + channel.getName() + " : You're not channel operator\r\n";
        send(sender->getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    channel.inviteClient(target);

    std::string msg = ":" + sender->getNickName() + " INVITE " + target->getNickName() + " " + channel._name + "\r\n";
    send(sender->getFd(), msg.c_str(), msg.length(), 0);
}

void Join(Channel &channel, Client *client)
{
    if (channel.isEmpty())
        channel.addOperator(client);
    
// // yahya sawb had lfunction
//     // hadchi li gal lia AI kadiro had function
// //"Your processChannelJoin function needs to take the clean channel name and key that I parsed, 
//     if (_vChannelNames.size() == 0)
//     {
//         _vChannelNames.push_back(channelName);
//     }
//     else
//     {
//         bool found = false;
//         for (size_t j = 0; j < _vChannelNames.size(); j++)
//         {
//             if (_vChannelNames[j] == channelName)
//             {
//                 found = true;
//                 break;
//             }
//         }
//         if (!found)
//             _vChannelNames.push_back(channelName);
//         else
//             return; 
//     }

//## What to tell Yahya in one sentence:
//decide whether to create a new channel or open an old one, 
    //====>> here we have to alloc for all one channel <========//
//     Channel *chan = new Channel();
//     chan->setName(channelName);
//     _vChannels.push_back(chan);
    
// //run your room security checks, and handle broadcasting the entry message to the clients."
// chan->broadcast("JOIN " + channelName + cmd.getMessage() + "\r\n", &client);
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
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    return NULL;
}

Channel* Server::getOrCreateChannel(const std::string& name)
{
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    Channel *chan = new Channel();
    chan->setName(name);
    _channels[name] = chan;
    return chan;
}
