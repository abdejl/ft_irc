#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include "../parsing/Client.hpp"


// class Client
// {
//     public:
//         int socket_fd;
//         std::string getNickName();
//         void send(std::string Message);
// };

class Channel
{
    private:
        std::string             _name;
        std::string             _topic;
        std::string             _key;

        bool                    _inviteOnly;
        bool                    _topicRestricted;

        int                     _userLimit;

        std::vector<Client*>    _vClient;
        std::vector<Client*>    _vOperator;
        std::vector<Client*>    _vInvitedClients;

    public:
        Channel();

        // getters
        std::string getName();
        std::string getTopic();
        std::string getKey();
        int         getUserLimit();

        // setters
        void setName(std::string name);
        void setTopic(std::string topic);
        void setTopicRestricted(bool value);
        void setInviteOnly(bool value);
        void setKey(std::string key);
        void setUserLimit(int l);

        // checks
        bool isEmpty();
        bool isOperator(Client *client);
        bool isTopicRestricted();
        bool isRestrictedTopic();
        bool isInviteOnly();
        bool hasKey();
        bool hasLimit();
        bool hasClient(Client *client);
        bool canJoin(Client *client, std::string key);
        bool isInvited(Client *client);
        bool hasDuplicateNickName(std::string nickname);

        // clients
        void addClient(Client *client);
        void removeClient(Client *client, bool printMSG);
        void broadcast(std::string message, Client *sender);

        // operators
        void addOperator(Client *client);
        void removeOperator(Client *client);

        // invitations
        void inviteClient(Client *client);
        void removeInvitation(Client *client);

        // commands
        void kickClient(Channel &channel, Client *sender, Client *target);
        void ChangeTopic(Channel &channel, Client *sender, std::string topic);
        void inviteToChannel(Channel &channel, Client *sender, Client *target);

        // utils
        std::vector<std::string> getClientList();
};

// non-member functions
void handleMode(Channel &channel, std::string mode, std::string arg);
void Join(Channel &channel, Client *client);

#endif

