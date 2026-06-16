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
    } else {
        std::cout << "DEBUG: Command ignored. Client not registered." << std::endl;
    }
}

void commandDispatcher::handleNick(Client &client, const Command &cmd, Server &server)
{
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
    std::cout << "TEST HANDLEPASS" << std::endl;
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
        std::cout << "Password is not Correct" << std::endl;
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
