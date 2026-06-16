#include "parsing/parser.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include "parsing/Client.hpp"
#include "parsing/cmdDispatcher.hpp"
#include "server/server.hpp"

std::vector<std::string> extractCommands(std::string &buffer)
{
    std::vector<std::string> commands;
    size_t pos;

    while ((pos = buffer.find("\r\n")) != std::string::npos)
    {
        commands.push_back(buffer.substr(0, pos));
        buffer.erase(0, pos + 2);
    }
    return commands;
}

Command parseCommand(const std::string &line) 
{
    Command cmd;
    size_t i = 0;
    std::string commandToBuild;

    if(line[0] == ':')
    {
        size_t prefix_end = line.find(' ');
        if(prefix_end != std::string::npos)
            cmd.setPrefix(line.substr(1, prefix_end - 1));
        i = prefix_end;
        while (i < line.size() && line[i] == ' ')
            i++;
        std::cout << "PREFIX: [" << cmd.getPrefix() << "]\n";
    }
    while (i < line.size() && line[i] != ' ')
    {
        commandToBuild += line[i++];
    }
    std::transform(commandToBuild.begin(), commandToBuild.end(), commandToBuild.begin(), ::toupper);
    cmd.setCommandName(commandToBuild);

    while (i < line.size() && line[i] == ' ')
        i++;

    std::string messageToBuild;
    std::vector<std::string> paramToBuild;

    while (i < line.size())
    {
        if (line[i] == ':')
        {
            messageToBuild = line.substr(i + 1);
            break;
        }
        std::string param;
        while (i < line.size() && line[i] != ' ')
        {
            param += line[i++];
        }
        paramToBuild.push_back(param);
        while (i < line.size() && line[i] == ' ')
            i++;
    }
    cmd.setMessage(messageToBuild);
    cmd.setParams(paramToBuild);

    return cmd;
}


int main(int argc, char **argv) 
{
    if (argc != 3)
    { 
        std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }
    Client              client;
    Server              server;
    commandDispatcher   dispatcher;

    server.setPort(argv[1]);
    server.setPassword(argv[2]);

    std::cout << "Enter IRC commands (e.g., PASS " << argv[2] << "):" << std::endl;

    CoreServer  ServerInfo(server.getPort());
    if (ServerInfo.PrepareServerSocket() != 1)
        return 1;
    while (true) 
    {
        if (ServerInfo.Receive(server) == -1)
            return 1;
        for (size_t i = 0; i < server.getClients().size(); i++)
        {
            std::string &clientBuffer = server.getClients()[i]->getBufferRef();
            std::vector<std::string> lines = extractCommands(clientBuffer); 
            for (size_t j = 0; j < lines.size(); j++)
            {
                Command cmd = parseCommand(lines[j]);
                dispatcher.execute(*server.getClients()[i], cmd, server);
            }
        }
    }
    return 0;
}
