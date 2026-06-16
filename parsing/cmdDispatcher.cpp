#include "cmdDispatcher.hpp"
#include "parser.hpp"
#include "../channel/channel.hpp"
#include "cmdDispatcher.hpp"
#include <sys/socket.h>

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
    } else {
        std::cout << "DEBUG: Command ignored. Client not registered." << std::endl;
    }
}

void commandDispatcher::handleNick(Client &client, const Command &cmd, Server &server)
{
    if(!client.getIsAuthenticated())
    {
        return;
    }
    if (cmd.getParams().empty())
    {
        std::string msg = "431 :No nickname given\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string newNick = cmd.getParams()[0];
    if (!client.checkIsValidNickname(newNick))
    {
        std::string msg = "432 " + newNick + " :Erroneous nickname\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        std::cout << "TEST HANDLENICK ERRONEOUS NICKNAME" << std::endl;
        return;
    }
    if (server.isNickInUse(newNick))
    {
        std::string msg = "433 * " + newNick + " :Nickname is already in use\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    client.setNickName(newNick);
    std::cout << "Client " << client.getFd() << " is now known as " << newNick << std::endl;
    tryCompleteRegistration(client);
}

void commandDispatcher::handlePass(Client &client, const Command &cmd, const Server& server)
{
    if(client.getIsAuthenticated() == true)
    {
        std::string err_msg = "462 :Unauthorized command (already registered)\r\n";
        send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
        return;
    }
    if(cmd.getParams().empty())
    {
        std::cout << "TEST EMPTY PARAMS" << std::endl;
        std::string err_msg = "461 " + client.getNickName() + " not enough parameters\r\n";
        send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
        return;
    }
    std::string userPass = cmd.getParams()[0];
    std::string correctPass = server.getPassword();
    if(userPass == correctPass)
    {
       client.setPassedPassword(true);
        std::cout << "Password Correct for client" << std::endl;
    }
    else 
    {
        std::string err_msg = "464 " + client.getNickName() + " :Password incorrect\r\n";
        send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
        std::cout << "Password is not Correct" << std::endl;
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
    if (!client.getIsAuthenticated()) // or client.getPassedPassword(), matching your flag name
    {
        std::cout << "DEBUG: Registration blocked. Client failed password authentication." << std::endl;
        return;
    }
    if (!client.getNickName().empty() && !client.getUserName().empty())
    {
        client.setIsRegistered(true);
        std::string welcome = "001 " + client.getNickName() + " :Welcome to the IRC Network!\r\n";
        send(client.getFd(), welcome.c_str(), welcome.length(), 0);
        std::cout << "Client " << client.getFd() << " is now fully registered." << std::endl;
    }

}

void commandDispatcher::handleUser(Client &client, const Command &cmd)
{
    if(!client.getIsAuthenticated())
    {
        return ;
    }
    if (client.getIsRegistered())
    {
        std::string msg = "462 :Unauthorized command (already registered)\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    if (cmd.getParams().size() < 3 || cmd.getMessage().empty())
    {
        std::string msg = "461 " + client.getNickName() + " USER :Not enough parameters\r\n";
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
        std::string msg = "461 " + client.getNickName() + " JOIN :Not enough parameters\r\n";
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
        if (channelName[0] != '#' && channelName[0] != '&')
        {
            std::string err_msg = "403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
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
        std::string msg = "411 :No recipient given (PRIVMSG)\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    if (cmd.getMessage().empty())
    {
        std::string msg = "412 :No text to send\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string target = cmd.getParams()[0];
    std::string text = cmd.getMessage();
    if (target[0] == '#' || target[0] == '&')
    {
        Channel* chan = server.getChannelByName(target);
        if (chan)
        {
            if (!chan->hasClient(&client))
            {
                std::string err = "442 " + client.getNickName() + " " + target + " :You're not on that channel\r\n";
                send(client.getFd(), err.c_str(), err.length(), 0);
                return;
            }
            std::string fullMsg = ":" + client.getNickName() + " PRIVMSG " + target + " :" + text + "\r\n";
            chan->broadcast(fullMsg, &client);
        }
        else
        {
            std::string err_msg = "401 " + client.getNickName() + " " + target + " :No such nick/channel\r\n";
            send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
        }
    }
}

void commandDispatcher::handleTopic(Client &client, const Command &cmd, Server &server)
{
    if (cmd.getParams().empty())
    {
        std::string msg = "461 " + client.getNickName() + " TOPIC :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string channelName = cmd.getParams()[0];
    Channel* chan = server.getChannelByName(channelName);
    if (!chan)
    {
        std::string err = "403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (!chan->hasClient(&client))
    {
        std::string err = "442 " + client.getNickName() + " " + channelName + " :You're not on that channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (cmd.getMessage().empty())
    {
        std::string topic = chan->getTopic();
        std::string reply;
        if (topic.empty())
            reply = "331 " + client.getNickName() + " " + channelName + " :No topic is set\r\n";
        else
            reply = "332 " + client.getNickName() + " " + channelName + " :" + topic + "\r\n";
        send(client.getFd(), reply.c_str(), reply.length(), 0);
        return;
    }
    chan->ChangeTopic(*chan, &client, cmd.getMessage());
}

void commandDispatcher::handleKick(Client &client, const Command &cmd, Server &server)
{
    if (cmd.getParams().size() < 2)
    {
        std::string msg = "461 " + client.getNickName() + " KICK :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string channelName = cmd.getParams()[0];
    std::string targetNick = cmd.getParams()[1];
    Channel* chan = server.getChannelByName(channelName);
    if (!chan)
    {
        std::string err = "403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (!chan->hasClient(&client))
    {
        std::string err = "442 " + client.getNickName() + " " + channelName + " :You're not on that channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    Client* target = server.getClientByNick(targetNick);
    if (!target)
    {
        std::string err = "401 " + client.getNickName() + " " + targetNick + " :No such nick/channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (!chan->hasClient(target))
    {
        std::string err = "441 " + client.getNickName() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    chan->kickClient(*chan, &client, target);
}

void commandDispatcher::handleInvite(Client &client, const Command &cmd, Server &server)
{
    if (cmd.getParams().size() < 2)
    {
        std::string msg = "461 " + client.getNickName() + " INVITE :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string targetNick = cmd.getParams()[0];
    std::string channelName = cmd.getParams()[1];
    Client* target = server.getClientByNick(targetNick);
    if (!target)
    {
        std::string err = "401 " + client.getNickName() + " " + targetNick + " :No such nick/channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    Channel* chan = server.getChannelByName(channelName);
    if (!chan)
    {
        std::string err = "403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (!chan->hasClient(&client))
    {
        std::string err = "442 " + client.getNickName() + " " + channelName + " :You're not on that channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (chan->hasClient(target))
    {
        std::string err = "443 " + client.getNickName() + " " + targetNick + " " + channelName + " :is already on channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    chan->inviteToChannel(*chan, &client, target);
}

void commandDispatcher::handleMode(Client &client, const Command &cmd, Server &server)
{
    if (cmd.getParams().empty())
    {
        std::string msg = "461 " + client.getNickName() + " MODE :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string channelName = cmd.getParams()[0];
    Channel* chan = server.getChannelByName(channelName);
    if (!chan)
    {
        std::string err = "403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (cmd.getParams().size() < 2)
    {
        std::string reply = "324 " + client.getNickName() + " " + channelName + " +nt\r\n";
        send(client.getFd(), reply.c_str(), reply.length(), 0);
        return;
    }
    if (!chan->isOperator(&client))
    {
        std::string err = "482 " + client.getNickName() + " " + channelName + " :You're not channel operator\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    std::string mode = cmd.getParams()[1];
    std::string modeArg = "";
    if (cmd.getParams().size() > 2)
        modeArg = cmd.getParams()[2];
    ::handleMode(*chan, mode, modeArg);
    std::string broadcast = ":" + client.getNickName() + " MODE " + channelName + " " + mode;
    if (!modeArg.empty())
        broadcast += " " + modeArg;
    broadcast += "\r\n";
    send(client.getFd(), broadcast.c_str(), broadcast.length(), 0);
    chan->broadcast(broadcast, &client);
}

void commandDispatcher::handlePart(Client &client, const Command &cmd, Server &server)
{
    if (cmd.getParams().empty())
    {
        std::string msg = "461 " + client.getNickName() + " PART :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string channelName = cmd.getParams()[0];
    Channel* chan = server.getChannelByName(channelName);
    if (!chan)
    {
        std::string err = "403 " + client.getNickName() + " " + channelName + " :No such channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    if (!chan->hasClient(&client))
    {
        std::string err = "442 " + client.getNickName() + " " + channelName + " :You're not on that channel\r\n";
        send(client.getFd(), err.c_str(), err.length(), 0);
        return;
    }
    chan->removeClient(&client, true);
}
