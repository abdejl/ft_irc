#include "cmdDispatcher.hpp"
#include "parser.hpp"
#include "../channel/channel.hpp"
#include <sys/socket.h>

static bool isChannelName(const std::string& name)
{
    return !name.empty() && (name[0] == '#' || name[0] == '&');
}

void commandDispatcher::execute(Client &client, const Command &cmd, Server &server)
{
    std::string name = cmd.getCommandName();

    if (name == "PASS")
        handlePass(client, cmd, server);
    else if (name == "CAP")
        handleCap(client, cmd);
    else if (name == "NICK")
        handleNick(client, cmd, server);
    else if (name == "USER")
        handleUser(client, cmd);
    else if (name == "PING")
    {
        std::string pong = "PONG :" + cmd.getMessage() + "\r\n";
        send(client.getFd(), pong.c_str(), pong.length(), 0);
    }
    else if (client.getIsRegistered())
    {
        if (name == "JOIN")
            handleJoin(client, cmd, server);
        else if (name == "PRIVMSG")
            handlePrivmsg(client, cmd, server);
        else if (name == "TOPIC")
            handleTopic(client, cmd, server);
        else if (name == "KICK")
            handleKick(client, cmd, server);
        else if (name == "INVITE")
            handleInvite(client, cmd, server);
        else if (name == "MODE")
            handleMode(client, cmd, server);
        else if (name == "PART")
            handlePart(client, cmd, server);
    }
    else
    {
        std::cout << "Client not registered." << std::endl;
    }
}

void commandDispatcher::handleNick(Client &client, const Command &cmd, Server &server)
{
    if (!client.getIsAuthenticated())
        return;
    if (cmd.getParams().empty())
    {
        std::string msg = ":localhost 431 :No nickname given\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string newNick = cmd.getParams()[0];
    if (!client.checkIsValidNickname(newNick))
    {
        std::string msg = ":localhost 432 " + newNick + " :Erroneous nickname\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    if (server.isNickInUse(newNick))
    {
        std::string msg = ":localhost 433 * " + newNick + " :Nickname is already in use\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    client.setNickName(newNick);
    std::cout << "Client " << client.getFd() << " is now known as " << newNick << std::endl;
    tryCompleteRegistration(client);
}

void commandDispatcher::handlePass(Client &client, const Command &cmd, const Server& server)
{
    if (client.getIsAuthenticated() == true)
    {
        std::string err_msg = ":localhost 462 :Unauthorized command (already registered)\r\n";
        send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
        return;
    }
    if (cmd.getParams().empty())
    {
        std::string err_msg = ":localhost 461 " + client.getNickName() + " PASS :Not enough parameters\r\n";
        send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
        return;
    }
    std::string userPass = cmd.getParams()[0];
    std::string correctPass = server.getPassword();
    if (userPass == correctPass)
        client.setPassedPassword(true);
    else
    {
        std::string err_msg = ":localhost 464 " + client.getNickName() + " :Password incorrect\r\n";
        send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
    }
}

void commandDispatcher::handleCap(Client &client, const Command &cmd)
{
    if (cmd.getParams().empty())
        return;
    std::string subcommand = cmd.getParams()[0];
    if (subcommand == "LS")
    {
        std::string msg = "CAP * LS :\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
    }
    else if (subcommand == "REQ")
    {
        std::string msg = "CAP * NAK :" + cmd.getMessage() + "\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
    }
    else if (subcommand == "END")
    {
        std::string msg = "CAP * END\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
    }
}

void commandDispatcher::tryCompleteRegistration(Client &client)
{
    if (client.getIsRegistered())
        return;
    if (!client.getIsAuthenticated())
    {
        std::cout << "DEBUG: Registration blocked — password not authenticated." << std::endl;
        return;
    }
    if (!client.getNickName().empty() && !client.getUserName().empty())
    {
        client.setIsRegistered(true);
        std::string welcome = ":localhost 001 " + client.getNickName() + " :Welcome to the IRC Network!\r\n";
        send(client.getFd(), welcome.c_str(), welcome.length(), 0);
        std::cout << "Client " << client.getFd() << " is now fully registered." << std::endl;
    }
}

void commandDispatcher::handleUser(Client &client, const Command &cmd)
{
    if (!client.getIsAuthenticated())
        return;
    if (client.getIsRegistered())
    {
        std::string msg = ":localhost 462 :Unauthorized command (already registered)\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    if (cmd.getParams().size() < 3 || cmd.getMessage().empty())
    {
        std::string msg = ":localhost 461 " + client.getNickName() + " USER :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    client.setUserName(cmd.getParams()[0]);
    client.setRealName(cmd.getMessage());
    tryCompleteRegistration(client);
}

void commandDispatcher::handleJoin(Client &client, const Command &cmd, Server &server)
{
    if (!client.getIsRegistered())
        return;
    if (cmd.getParams().empty())
    {
        std::string msg = ":localhost 461 " + client.getNickName() + " JOIN :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::vector<std::string> channels = splitByComma(cmd.getParams()[0]);
    std::vector<std::string> keys;
    if (cmd.getParams().size() > 1)
        keys = splitByComma(cmd.getParams()[1]);
    for (size_t i = 0; i < channels.size(); i++)
    {
        std::string channelName = channels[i];
        std::string key = (i < keys.size()) ? keys[i] : "";
        if (!isChannelName(channelName))
        {
            std::string err_msg = ":localhost 403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
            send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
            continue;
        }
        server.processChannelJoin(client, channelName, key);
    }
}

void commandDispatcher::handlePrivmsg(Client &client, const Command &cmd, Server &server)
{
    if (!client.getIsRegistered())
        return;
    if (cmd.getParams().empty())
    {
        std::string msg = ":localhost 461 :No recipient given (PRIVMSG)\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    if (cmd.getMessage().empty())
    {
        std::string msg = ":localhost 412 :No text to send\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string target = cmd.getParams()[0];
    std::string text = cmd.getMessage();
    if (isChannelName(target))
    {
        Channel* chan = server.getChannelByName(target);
        if (chan)
        {
            if (!chan->hasClient(&client))
            {
                std::string err = ":localhost 442 " + client.getNickName() + " " + target + " :You're not on that channel\r\n";
                send(client.getFd(), err.c_str(), err.length(), 0);
                return;
            }
            std::string fullMsg = ":" + client.getHostmask() + " PRIVMSG " + target + " :" + text + "\r\n";
            chan->broadcast(fullMsg, &client);
        }
        else
        {
            std::string err_msg = ":localhost 401 " + client.getNickName() + " " + target + " :No such nick/channel\r\n";
            send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
        }
    }
    else
    {
        Client* recipient = server.getClientByNick(target);
        if (!recipient)
        {
            std::string err_msg = ":localhost 401 " + client.getNickName() + " " + target + " :No such nick/channel\r\n";
            send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
            return;
        }
        std::string fullMsg = ":" + client.getHostmask() + " PRIVMSG " + target + " :" + text + "\r\n";
        send(recipient->getFd(), fullMsg.c_str(), fullMsg.length(), 0);
    }
}

void commandDispatcher::handleTopic(Client &client, const Command &cmd, Server &server)
{
    if (cmd.getParams().empty())
    {
        std::string msg = ":localhost 461 " + client.getNickName() + " TOPIC :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string channelName = cmd.getParams()[0];
    Channel* chan = server.getChannelByName(channelName);
    if (!chan)
    {
        std::string err = ":localhost 403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (!chan->hasClient(&client))
    {
        std::string err = ":localhost 442 " + client.getNickName() + " " + channelName + " :You're not on that channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (cmd.getMessage().empty())
    {
        std::string topic = chan->getTopic();
        std::string reply;
        if (topic.empty())
            reply = ":localhost 331 " + client.getNickName() + " " + channelName + " :No topic is set\r\n";
        else
            reply = ":localhost 332 " + client.getNickName() + " " + channelName + " :" + topic + "\r\n";
        send(client.getFd(), reply.c_str(), reply.length(), 0);
        return;
    }
    chan->ChangeTopic(&client, cmd.getMessage());
}

void commandDispatcher::handleKick(Client &client, const Command &cmd, Server &server)
{
    if (cmd.getParams().size() < 2)
    {
        std::string msg = ":localhost 461 " + client.getNickName() + " KICK :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string channelName = cmd.getParams()[0];
    std::string targetNick = cmd.getParams()[1];
    Channel* chan = server.getChannelByName(channelName);
    if (!chan)
    {
        std::string err = ":localhost 403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (!chan->hasClient(&client))
    {
        std::string err = ":localhost 442 " + client.getNickName() + " " + channelName + " :You're not on that channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    Client* target = server.getClientByNick(targetNick);
    if (!target)
    {
        std::string err = ":localhost 401 " + client.getNickName() + " " + targetNick + " :No such nick/channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (!chan->hasClient(target))
    {
        std::string err = ":localhost 441 " + client.getNickName() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    // BUG FIX #3: was chan->kickClient(*chan, &client, target)
    // The redundant Channel& parameter is removed — chan is already `this`
    chan->kickClient(&client, target);
}

void commandDispatcher::handleInvite(Client &client, const Command &cmd, Server &server)
{
    if (cmd.getParams().size() < 2)
    {
        std::string msg = ":localhost 461 " + client.getNickName() + " INVITE :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string targetNick = cmd.getParams()[0];
    std::string channelName = cmd.getParams()[1];
    Client* target = server.getClientByNick(targetNick);
    if (!target)
    {
        std::string err = ":localhost 401 " + client.getNickName() + " " + targetNick + " :No such nick/channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    Channel* chan = server.getChannelByName(channelName);
    if (!chan)
    {
        std::string err = ":localhost 403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (!chan->hasClient(&client))
    {
        std::string err = ":localhost 442 " + client.getNickName() + " " + channelName + " :You're not on that channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (chan->hasClient(target))
    {
        std::string err = ":localhost 443 " + client.getNickName() + " " + targetNick + " " + channelName + " :is already on channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    // BUG FIX #3: was chan->inviteToChannel(*chan, &client, target)
    chan->inviteToChannel(&client, target);
}

void commandDispatcher::handleMode(Client &client, const Command &cmd, Server &server)
{
    if (cmd.getParams().empty())
    {
        std::string msg = ":localhost 461 " + client.getNickName() + " MODE :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string target = cmd.getParams()[0];
    if (!isChannelName(target))
    {
        if (cmd.getParams().size() < 2)
        {
            std::string reply = ":localhost 221 " + client.getNickName() + " :+i\r\n";
            send(client.getFd(), reply.c_str(), reply.length(), 0);
        }
        else if (target != client.getNickName())
        {
            std::string err = ":localhost 502 " + client.getNickName() + " :Cannot change mode for other users\r\n";
            send(client.getFd(), err.c_str(), err.length(), 0);
        }
        else
        {
            std::string reply = ":localhost 221 " + client.getNickName() + " :+i\r\n";
            send(client.getFd(), reply.c_str(), reply.length(), 0);
        }
        return;
    }
    Channel* chan = server.getChannelByName(target);
    if (!chan)
    {
        std::string err = ":localhost 403 " + client.getNickName() + " " + target + " :No such channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (cmd.getParams().size() < 2)
    {
        std::string reply = ":localhost 324 " + client.getNickName() + " " + target + " +nt\r\n";
        send(client.getFd(), reply.c_str(), reply.length(), 0);
        return;
    }
    if (!chan->isOperator(&client))
    {
        std::string err = ":localhost 482 " + client.getNickName() + " " + target + " :You're not channel operator\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    std::string mode = cmd.getParams()[1];
    std::string modeArg = "";
    if (cmd.getParams().size() > 2)
        modeArg = cmd.getParams()[2];
    // BUG FIX #1: handleMode() (the global helper in channel.cpp) now strips
    // trailing \r\n from modeArg before applying it, so the channel state
    // is never corrupted by network delimiter artifacts.
    ::handleMode(*chan, mode, modeArg);
    std::string broadcastMsg = ":" + client.getHostmask() + " MODE " + target + " " + mode;
    if (!modeArg.empty())
        broadcastMsg += " " + modeArg;
    broadcastMsg += "\r\n";
    send(client.getFd(), broadcastMsg.c_str(), broadcastMsg.length(), 0);
    chan->broadcast(broadcastMsg, &client);
}

void commandDispatcher::handlePart(Client &client, const Command &cmd, Server &server)
{
    if (cmd.getParams().empty())
    {
        std::string msg = ":localhost 461 " + client.getNickName() + " PART :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string channelName = cmd.getParams()[0];
    Channel* chan = server.getChannelByName(channelName);
    if (!chan)
    {
        std::string err = ":localhost 403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (!chan->hasClient(&client))
    {
        std::string err = ":localhost 442 " + client.getNickName() + " " + channelName + " :You're not on that channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    chan->removeClient(&client, true);
}
