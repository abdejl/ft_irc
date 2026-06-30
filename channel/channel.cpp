#include "channel.hpp"
#include <sys/socket.h>


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
        if ((*it)->getFd() == client->getFd())
        {
            if (printMSG)
            {
                std::string partMsg = ":" + client->getHostmask() + " PART " + _name + "\r\n";
                broadcast(partMsg, client);
            }
            _vClient.erase(it);
            break;
        }
    }
}

// BUG FIX #2 (Broadcast Raw Forwarder):
// broadcast() was previously hardcoding a PRIVMSG prefix onto every message.
// It is now a pure raw forwarder — it sends exactly the string it receives.
// Callers (kickClient, ChangeTopic, inviteToChannel, removeClient) are
// responsible for building the full correctly-formatted IRC line themselves.
// sender == NULL means send to ALL members (used for server-initiated messages).
void Channel::broadcast(std::string message, Client *sender)
{
    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
    {
        if (sender && (*it)->getFd() != sender->getFd())
            send((*it)->getFd(), message.c_str(), message.length(), 0);
        else if (!sender)
            send((*it)->getFd(), message.c_str(), message.length(), 0);
    }
}

void Channel::addOperator(Client *client)
{
    _vOperator.push_back(client);
}

void Channel::removeOperator(Client *client)
{
    for (std::vector<Client*>::iterator it = _vOperator.begin(); it != _vOperator.end(); it++)
    {
        if ((*it)->getFd() == client->getFd())
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

// BUG FIX #1 (\r\n strip in handleMode):
// Mode arguments arriving from the network (e.g. keys, limit values) may still
// carry a trailing \r from the raw TCP stream if the parser ever lets one
// slip through. std::atoi() silently ignores \r, but string comparisons for
// keys like (_key != key) will FAIL because "\r" changes the string value.
// We strip trailing \r and \n from the arg before applying it.
void handleMode(Channel &channel, std::string mode, std::string arg, Client *target)
{
    // remove \r\n
    while (!arg.empty() && (arg[arg.size() - 1] == '\r' || arg[arg.size() - 1] == '\n'))
        arg.erase(arg.size() - 1);

    if (mode == "+i")
        channel.setInviteOnly(true);
    else if (mode == "-i")
        channel.setInviteOnly(false);
    else if (mode == "+k")
        channel.setKey(arg);
    else if (mode == "-k")
        channel.setKey("");
    else if (mode == "+t")
        channel.setTopicRestricted(true);
    else if (mode == "-t")
        channel.setTopicRestricted(false);
    else if (mode == "+l")
        channel.setUserLimit(std::atoi(arg.c_str()));
    else if (mode == "-l")
        channel.setUserLimit(-1);
    else if (mode == "+o")
    {
        if (target)
            channel.addOperator(target);
    }
    else if (mode == "-o")
    {
        if (target)
            channel.removeOperator(target);
    }
}

bool Channel::canJoin(Client *client, std::string key)
{
    // Strip any trailing \r\n from the key the client sent
    while (!key.empty() && (key[key.size() - 1] == '\r' || key[key.size() - 1] == '\n'))
        key.erase(key.size() - 1);

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
        if ((*it)->getFd() == client->getFd())
            return true;
    }
    return false;
}

void Channel::removeInvitation(Client *client)
{
    for (std::vector<Client*>::iterator it = _vInvitedClients.begin(); it != _vInvitedClients.end(); it++)
    {
        if ((*it)->getFd() == client->getFd())
        {
            _vInvitedClients.erase(it);
            return;
        }
    }
}

// BUG FIX #3 (Redundant Parameter / Self-Reference):
// kickClient was declared as a member of Channel but received a Channel& parameter,
// creating a dual-lookup layer (this-> vs channel.) on the same object.
// Fixed: removed the redundant Channel& parameter. The method now operates
// entirely through `this`, which is the channel instance the dispatcher already
// retrieved via getChannelByName(). The call site in handleKick is updated
// from chan->kickClient(*chan, ...) to chan->kickClient(...).
void Channel::kickClient(Client *sender, Client *target)
{
    if (!isOperator(sender))
    {
        std::string msg = ":localhost 482 " + sender->getNickName() + " " + _name + " :You're not channel operator\r\n";
        send(sender->getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    // Send KICK to the target being kicked
    std::string kickMsg = ":" + sender->getHostmask() + " KICK " + _name + " " + target->getNickName() + " :kicked\r\n";
    send(target->getFd(), kickMsg.c_str(), kickMsg.length(), 0);
    // Broadcast KICK to remaining channel members
    broadcast(kickMsg, target);
    removeClient(target, false);
    removeOperator(target);
}

bool Channel::isRestrictedTopic()
{
    return _topicRestricted;
}

void Channel::ChangeTopic(Client *sender, std::string topic)
{
    if (!sender)
        return;
    // Strip trailing \r\n from topic text arriving from the network
    while (!topic.empty() && (topic[topic.size() - 1] == '\r' || topic[topic.size() - 1] == '\n'))
        topic.erase(topic.size() - 1);
    if (isRestrictedTopic() && !isOperator(sender))
    {
        std::string msg = ":localhost 482 " + sender->getNickName() + " " + _name + " :You're not channel operator\r\n";
        send(sender->getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    setTopic(topic);
    std::string topicMsg = ":" + sender->getHostmask() + " TOPIC " + _name + " :" + topic + "\r\n";
    // Send to the sender explicitly (broadcast excludes sender)
    send(sender->getFd(), topicMsg.c_str(), topicMsg.length(), 0);
    broadcast(topicMsg, sender);
}

// BUG FIX #3 (Redundant Parameter / Self-Reference):
// Same issue as kickClient — inviteToChannel received a Channel& in addition
// to being a Channel member. Removed the redundant parameter.
// Also fixed: inviteToChannel was calling addClient() directly, bypassing
// canJoin() checks and the operator-first logic. INVITE should only mark
// the client as invited; the actual JOIN happens when the client sends JOIN.
void Channel::inviteToChannel(Client *sender, Client *target)
{
    if (!isOperator(sender))
    {
        std::string msg = ":localhost 482 " + sender->getNickName() + " " + _name + " :You're not channel operator\r\n";
        send(sender->getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    // Mark as invited — the client will JOIN normally, canJoin() will pass for them
    inviteClient(target);

    // Notify the sender that the invite was sent
    std::string inviteMsg = ":" + sender->getHostmask() + " INVITE " + target->getNickName() + " " + _name + "\r\n";
    send(sender->getFd(), inviteMsg.c_str(), inviteMsg.length(), 0);
    // Notify the target
    send(target->getFd(), inviteMsg.c_str(), inviteMsg.length(), 0);
}

// BUG FIX (JOIN Order of Operations):
// The original code called addClient() BEFORE the isEmpty() check, so the
// channel was never empty when we checked whether to make the joiner an operator.
// Fixed: check isEmpty() FIRST, then addClient().
void Server::processChannelJoin(Client &client, std::string channelName, std::string key)
{
    Channel *channel = getOrCreateChannel(channelName);
    if (!channel)
        return;
    if (channel->hasClient(&client))
        return;
    if (!channel->canJoin(&client, key))
    {
        std::string msg = ":localhost 475 " + client.getNickName() + " " + channelName + " :Cannot join channel (+k or +l)\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    // isEmpty() MUST be checked before addClient()
    bool wasEmpty = channel->isEmpty();
    channel->addClient(&client);
    if (wasEmpty)
        channel->addOperator(&client);

    std::string joinMsg = ":" + client.getHostmask() + " JOIN " + channelName + "\r\n";
    // Send JOIN to the joining client
    send(client.getFd(), joinMsg.c_str(), joinMsg.length(), 0);
    // Broadcast JOIN to existing members (excludes the sender)
    channel->broadcast(joinMsg, &client);
}

bool Channel::hasClient(Client *client)
{
    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
    {
        if ((*it)->getFd() == client->getFd())
            return true;
    }
    return false;
}

std::vector<std::string> Channel::getClientList()
{
    std::vector<std::string> clientList;
    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
        clientList.push_back((*it)->getNickName());
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
    if (name.empty() || (name[0] != '#' && name[0] != '&'))
        return NULL;
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