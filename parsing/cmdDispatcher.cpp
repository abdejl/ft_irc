#include "cmdDispatcher.hpp"
#include "parser.hpp"
#include "../channel/channel.hpp"

#include "cmdDispatcher.hpp"
#include <sys/socket.h> // For send()

void commandDispatcher::execute(Client &client, const Command &cmd, Server &server) 
{
    std::string name = cmd.getCommandName();

    if (name == "PASS")
        handlePass(client, cmd, server);
    else if (name == "NICK")
        handleNick(client, cmd, server);
    else if (name == "USER")
        handleUser(client, cmd);
    else if (client.getIsRegistered()) { // Only allow if registered
        if (name == "JOIN")
            handleJoin(client, cmd);
        else if (name == "PRIVMSG")
            handlePrivmsg(client, cmd, server);
    } else {
        std::cout << "DEBUG: Command ignored. Client not registered." << std::endl;
    }
}

void commandDispatcher::handleNick(Client &client, const Command &cmd, Server &server)
{
    // 1. Check if the user has passed the password check first
    if (!client.getIsAuthenticated())
    {
        return;
    }

    // 2. Check: Is the nickname parameter missing? (Numeric 431) 
    if (cmd.getParams().empty())
    {
        std::string msg = "431 :No nickname given\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    std::string newNick = cmd.getParams()[0];

    // 3. Check: Is the nickname valid (format-wise)? (Numeric 432)
    if (!client.checkIsValidNickname(newNick))
    {
        std::string msg = "432 " + newNick + " :Erroneous nickname\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        std::cout << "TEST HANDLENICK ERRONEOUS NICKNAME" << std::endl;
        return;
    }

    // 4. Check: Is the nickname already in use? (Numeric 433) 
    if (server.isNickInUse(newNick))
    {
        std::string msg = "433 * " + newNick + " :Nickname is already in use\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }
    // if (server.isNickInUse(newNick, client.getFd()))
    // {
    //     std::string msg = "433 * " + newNick + " :Nickname is already in use\r\n";
    //     send(client.getFd(), msg.c_str(), msg.length(), 0);
    //     return;
    // }
    // 5. Success: Update the nickname
    client.setNickName(newNick);
    std::cout << "Client " << client.getFd() << " is now known as " << newNick << std::endl;
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
        std::cout << "Password is not Correct" << std::endl;
}


void commandDispatcher::handleUser(Client &client, const Command &cmd)
{
    // 1. Check if the user has provided a password first
    if (!client.getIsAuthenticated())
    {
        return;
    }

    // 2. Check if the user is already registered (462)
    if (client.getIsRegistered())
    {
        std::string msg = "462 :Unauthorized command (already registered)\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }

    // 3. Verify exactly 4 parameters are present (461)
    // USER <username> <hostname> <servername> :<realname>
    // my parser stores the trailing message (realname) separately.
    if (cmd.getParams().size() < 3 || cmd.getMessage().empty())
    {
        std::string msg = "461 " + client.getNickName() + " USER :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }

    // 4. extract and set data
    client.setUserName(cmd.getParams()[0]);
    client.setRealName(cmd.getMessage());

    // Once PASS, NICK, and USER are done the user is Registered
    if (!client.getNickName().empty() && client.getNickName() != "default")
    {
        client.setIsRegistered(true);
        
        // send welcome message
        std::string welcome = "001 " + client.getNickName() + " :Welcome to the IRC Network!\r\n";
        send(client.getFd(), welcome.c_str(), welcome.length(), 0);
        
        std::cout << "Client " << client.getFd() << " is now fully registered." << std::endl;
    }
}

void commandDispatcher::handleJoin(Client &client, const Command &cmd)
{
    // 1. Check if the user is fully registered 
    if (!client.getIsRegistered())
    {
        return;
    }

    // 2. Check for parameters (Numeric 461)
    if (cmd.getParams().empty())
    {
        std::string msg = "461 " + client.getNickName() + " JOIN :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }

    // 3. handle multiple channels (e.g., JOIN #chan1,#chan2)
    std::vector<std::string> channels = splitByComma(cmd.getParams()[0]);
    std::vector<std::string> keys;
    
    if (cmd.getParams().size() > 1)
        keys = splitByComma(cmd.getParams()[1]);

    for (size_t i = 0; i < channels.size(); i++)
    {
        std::string channelName = channels[i];
        std::string key = (i < keys.size()) ? keys[i] : "";

        // Valid channel names must start with # or &
        if (channelName[0] != '#' && channelName[0] != '&')
        {
            // Send error 403
            std::string err_msg = "403: JOIN not valid channel";
            send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
            continue;
        }
        Channel channel;
        if (channel.isEmpty())
        {
            std::cout << "TEST CHANNEL" << std::endl;
            channel.addOperator(&client);
        }
        // YAHYA PART:
        // ZIYR M3ANA:
    }
}

void commandDispatcher::handlePrivmsg(Client &client, const Command &cmd, Server &server)
{
    if (!client.getIsRegistered()) return;

    // 1. Check for target (Numeric 411)
    if (cmd.getParams().empty())
    {
        std::string msg = "411 :No recipient given (PRIVMSG)\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }

    // 2. Check for the true text (Numeric 412) [cite: 151]
    if (cmd.getMessage().empty())
    {
        std::string msg = "412 :No text to send\r\n";
        send(client.getFd(), msg.c_str(), msg.length(), 0);
        return;
    }

    std::string target = cmd.getParams()[0];
    std::string text = cmd.getMessage();

    // 3. Routing the message
    if (target[0] == '#' || target[0] == '&')
    {
        // Target is a CHANNEL
        // salam '-'
        // YAHYA your part is here...
    }
    else
    {
        // Target is a USER
        // Find the user in the server's _clients vector
        Client* targetClient = server.getClientByNick(target);
        if (targetClient)
        {
            std::string fullMsg = ":" + client.getNickName() + " PRIVMSG " + target + " :" + text + "\r\n";
            send(targetClient->getFd(), fullMsg.c_str(), fullMsg.length(), 0);
        }
        else 
        {
            std::string err_msg = "401: " + client.getNickName() + " no such nickName" ;
            send(client.getFd(), err_msg.c_str(), err_msg.length(), 0);
            // Send error 401 (ERR_NOSUCHNICK)
        }
    }
}
