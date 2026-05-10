#include "parser.hpp"
#include <iostream>
#include <vector>
#include <string>
#include "Client.hpp"
#include "cmdDispatcher.hpp"

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

    // 1. Get command
    if(line[0] == ':')
    {
        size_t prefix_end = line.find(' ');
        if(prefix_end != std::string::npos)
            cmd.setPrefix(line.substr(1, prefix_end - 1));
        i = prefix_end;
        std::cout << "PREFIX: [" << cmd.getPrefix() << "]\n";
    }
    while (i < line.size() && line[i] != ' ')
    {
        commandToBuild += line[i++];
    }
    cmd.setCommandName(commandToBuild);

    // skip spaces
    while (i < line.size() && line[i] == ' ')
        i++;

    // 2. Parse params + trailing message
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

    Server server;
    server.setPort(argv[1]);
    server.setPassword(argv[2]);
    
    Client client;
    client.setFd(1); // Standard output for testing
    
    commandDispatcher dispatcher;
    std::string buffer;
    std::string input;

    std::cout << "--- IRC Parser Debug Mode ---" << std::endl;
    std::cout << "Password set to: " << server.getPassword() << std::endl;
    std::cout << "Enter IRC commands (e.g., PASS " << argv[2] << "):" << std::endl;

    while (std::getline(std::cin, input)) 
    {
        if (input == "exit") break;

        // Simulate network behavior by adding \r\n
        buffer += input + "\r\n"; 
        std::vector<std::string> lines = extractCommands(buffer);

        for (size_t i = 0; i < lines.size(); i++) 
        {
            Command cmd = parseCommand(lines[i]);
            //Pass the server by reference so handlers can see the password
            dispatcher.execute(client, cmd, server); 
        }
    }
    return 0;
}
