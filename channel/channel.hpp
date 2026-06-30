#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include "../parsing/Client.hpp"

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
        void inviteToChannel(Client *sender, Client *target);
       void removeInvitation(Client *client);

        // BUG FIX #3: Removed redundant Channel& parameter from both methods.
        // They are member functions — they already have access to `this`.
        void kickClient(Client *sender, Client *target);
        void ChangeTopic(Client *sender, std::string topic);
        // utils
        std::vector<std::string> getClientList();
};

void handleMode(Channel &channel, std::string mode, std::string arg, Client *target);
// non-member helper — applies a single mode string to a channel
// void handleMode(Channel &channel, std::string mode, std::string arg);

// void handleMode(Channel &channel, std::string mode, std::string arg, Client *target);
#endif