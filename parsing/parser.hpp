#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>

class Command{
private:
    std::string commandName;
    std::vector<std::string> params;
    std::string message;
    std::string _prefix;

public:
    void setCommandName(std::string cmdName);
    void setParams(std::vector<std::string> param);
    void setMessage(std::string msg);
    std::string getCommandName() const;
    std::vector<std::string> getParams() const;
    std::string getMessage() const;
    void setPrefix(std::string prefix);
    std::string getPrefix() const;
};
std::vector<std::string> splitByComma(std::string str);


#endif
