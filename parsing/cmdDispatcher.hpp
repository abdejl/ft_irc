#ifndef CMDDISPATCHER_HPP
#define CMDDISPATCHER_HPP

#include "Client.hpp"

class commandDispatcher{
private:

public:
    void execute(Client &client, const Command &cmd);
    void handleNick(Client &client, const Command &cmd);
    void handlePass(Client &client, const Command &cmd, const Server& server);
    void handleUser(Client &client, const Command &cmd);
    void handlJoin(Client &client, const Command &cmd);
    void handlePrivmsg(Client &client, const Command &cmd);
};

#endif // !
