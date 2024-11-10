#pragma once

#include <map>

class CommandLineParser {

public:
    CommandLineParser();
    
    void add_bool(std::string shortName, std::string longName, bool def, bool required);
    void add_int(std::string shortName, std::string longName, int def, bool required);
    void add_string(std::string shortName, std::string longName, std::string def, bool required);

};