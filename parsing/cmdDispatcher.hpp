#ifndef CMDDISPATCHER_HPP
#define CMDDISPATCHER_HPP

#include "Client.hpp"
#include <sys/types.h>
#include <sys/socket.h>

class commandDispatcher{
private:

public:
    void execute(Client &client, const Command &cmd, Server &server);
    void handleNick(Client &client, const Command &cmd, Server &server);
    void handlePass(Client &client, const Command &cmd, const Server& server);
    void handleUser(Client &client, const Command &cmd);
    void handleJoin(Client &client, const Command &cmd, Server &server);
    void handlePrivmsg(Client &client, const Command &cmd, Server &server);
};

#endif
