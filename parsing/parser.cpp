#include "parser.hpp"

void Command::setCommandName(std::string cmdName)
{
    this->commandName = cmdName;
}

void Command::setParams(std::vector<std::string> param)
{
    this->params = param;
}

void Command::setMessage(std::string msg)
{
    this->message = msg;
}


std::string Command::getCommandName() const
{
    return(this->commandName);
}

std::vector<std::string> Command::getParams() const
{
    return(this->params);
}

std::string Command::getMessage() const
{
    return(this->message);
}

void Command::setPrefix(std::string prefix)
{
    this->_prefix = prefix;
}

std::string Command::getPrefix() const
{
    return(this->_prefix);
}
