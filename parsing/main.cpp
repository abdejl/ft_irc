#include "parser.hpp"
#include <iostream>
#include <vector>
#include <string>
#include "Client.hpp"

std::vector<std::string> extractCommands(std::string &buffer) {
    std::vector<std::string> commands;
    size_t pos;

    while ((pos = buffer.find("\r\n")) != std::string::npos) {
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
    std::string input;
    std::vector<std::string> cmds; 
    std::vector<std::string> newcmds;
    // command args;
    std::string buffer;

    Server server;
    server.setPort(argv[1]);
    server.setPassword(argv[2]);
    if(argc == 2)
    {
        while (true)
        {
            std::getline(std::cin, input);

            if (input == "exit")
                break;

            buffer += input + "\r\n";

            std::vector<std::string> newcmds = extractCommands(buffer);


            for (size_t i = 0; i < newcmds.size(); i++)
            {
                cmds.push_back(newcmds[i]);
            }
            for (size_t i = 0; i < newcmds.size(); i++) 
            {
                Command c = parseCommand(newcmds[i]);
                std::cout << "CMD: " << c.getCommandName() << "\n";

                for (size_t j = 0; j < c.getParams().size(); j++)
                    std::cout << "param: [" << c.getParams()[j] << "]\n";

                std::cout << "trailing: [" << c.getMessage() << "]\n";

                std::cout << "------\n";
            }
    
        }
    }
    std::cout << "Wrong Number Of Arguments" << std::endl;
}
