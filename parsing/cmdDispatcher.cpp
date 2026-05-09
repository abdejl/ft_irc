#include "cmdDispatcher.hpp"


void commandDispatcher::execute(Client &client, const Command &cmd)
{
    std::string name = cmd.getCommandName();
    Server server;
    if(name == "PASS")
        handlePass(client, cmd, server);
    
    else if(name == "NICK")
        handleNick(client, cmd);
    
    else if(name == "USER")
        handleUser(client, cmd);

    else if(name == "JOIN")
        handlJoin(client, cmd);

    else if(name == "PRIVMSG")
        handlePrivmsg(client, cmd);

}

void handleNick(Client &client, const Command &cmd)
{
    std::string nickName = cmd.getCommandName();
   client.setNickName(nickName);
}

void handlePass(Client &client, const Command &cmd, const Server& server)
{
    std::string userPass = cmd.getParams()[0];
    std::string correctPass = server.getPassword();
    if(userPass == correctPass)
       client.setPassedPassword(true);
}

void handleUser(Client &client, const Command &cmd)
{
    std::string userName = cmd.getCommandName();
    client.setUserName(userName);
}

void handlJoin(Client &client, const Command &cmd)
{

}

void handlePrivmsg(Client &client, const Command &cmd)
{

}
