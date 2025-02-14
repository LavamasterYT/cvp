/**
 * @file config.h
 * @brief A simple single header file that reads configuration files.
 * @version 0.1
 * @date 2025-02-14
 * 
 * @copyright Copyright (c) 2025
 * 
 * The purpose of this header is to read and parse an app configuration file across different
 * operating systems (Windows, Linux, macOS).
 * 
 * I am sure that there are other libraries or files that does this exact thing if not better,
 * however I am doing this just to learn some more C++.
 * 
 * This header library reads the configuration file that is located on ~/.config/appname/file.conf
 * on Unix or %localappdata%\appname\file.conf on Windows, and parses it to set variables passed in
 * by the program.
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

namespace config {
    enum config_err {
        CONFIG_SUCCESS,
        CONFIG_ERR_FILE_NOT_FOUND,
        CONFIG_ERR_KEY_TYPE_MISMATCH,
        CONFIG_ERR_INVALID_KEYNAME,
        CONFIG_ERR_SYNTAX_ERROR,
        CONFIG_ERR_KEY_EXISTS,
        CONFIG_ERR_OPTION_EXISTS
    };

    class conffile {
        std::map<std::string, bool*> boolean_keys;
        std::map<std::string, int*> integer_keys;
        std::map<std::string, std::string*> string_keys;

        std::filesystem::path path;
        std::string last_err;

        int error(config_err error, int line, std::string msg = "", std::string msg2 = "") {
            switch (error) {
            case CONFIG_SUCCESS:
                last_err = "";
                return error;
            case CONFIG_ERR_FILE_NOT_FOUND:
                last_err = "File not found: \"" + msg + "\".";
                return error;
            case CONFIG_ERR_KEY_TYPE_MISMATCH:
                last_err = "Syntax error line " + std::to_string(line) + ": value \"" + msg + "\" does not match expected key data type \"" + msg2 + "\".";
                return error;
            case CONFIG_ERR_INVALID_KEYNAME:
                last_err = "Syntax error line " + std::to_string(line) + ": invalid key \"" + msg + "\".";
                return error;
            case CONFIG_ERR_SYNTAX_ERROR:
                last_err = "Syntax error line " + std::to_string(line) + ": cannot find '=' for assignment.";
                return error;
            case CONFIG_ERR_KEY_EXISTS:
                last_err = "Syntax error line " + std::to_string(line) + ": key \"" + msg + "\" is already defined.";
                return error;
            case CONFIG_ERR_OPTION_EXISTS:
                last_err = "Option \"" + msg + "\" already exists.";
                return error;
            }
        }

    public:
        /**
         * @brief Initializes a parser for a specific configuration file.
         * 
         * @param appname The name of the application, or subdirectory to look for.
         * @param conf The name of the .conf file to open.
         */
        conffile(std::string appname, std::string conf) {
            #ifdef _WIN32
            const char* localdata = std::getenv("LOCALAPPDATA");
            if (localdata)
                path = std::filesystem::path(localdata) / appname / (conf + ".conf");
            #else
            const char* home = std::getenv("HOME");
            if (home)
                path = std::filesystem::path(home) / ".config" / appname / (conf + ".conf");
            #endif
            else
                path = "";
        }

        /**
         * @brief Gets the last reported error from the parser, if any.
         * 
         * @return std::string 
         */
        std::string get_last_error() {
            return last_err;
        }

        /**
         * @brief Adds an option for the configurator to parse.
         * 
         * @param key The name of the key to read.
         * @param variable A reference on where to assign the value of the key.
         * @return int 0 on success, config_err on error.
         */
        int add_option(std::string key, bool &variable) {
            if (integer_keys.find(key) != integer_keys.end())
                return error(CONFIG_ERR_OPTION_EXISTS, 0, key);
            if (string_keys.find(key) != string_keys.end())
                return error(CONFIG_ERR_OPTION_EXISTS, 0, key);
            if (boolean_keys.find(key) != boolean_keys.end())
                return error(CONFIG_ERR_OPTION_EXISTS, 0, key);
            else
                boolean_keys.insert({key, &variable});

            return error(CONFIG_SUCCESS, 0);
        }

        int add_option(std::string key, int &variable) {
            if (boolean_keys.find(key) != boolean_keys.end())
                return error(CONFIG_ERR_OPTION_EXISTS, 0, key);
            if (string_keys.find(key) != string_keys.end())
                return error(CONFIG_ERR_OPTION_EXISTS, 0, key);
            if (integer_keys.find(key) != integer_keys.end())
                return error(CONFIG_ERR_OPTION_EXISTS, 0, key);
            else
                integer_keys.insert({key, &variable});

            return error(CONFIG_SUCCESS, 0);
        }

        int add_option(std::string key, std::string &variable) {
            if (integer_keys.find(key) != integer_keys.end())
                return error(CONFIG_ERR_OPTION_EXISTS, 0, key);
            if (boolean_keys.find(key) != boolean_keys.end())
                return error(CONFIG_ERR_OPTION_EXISTS, 0, key);
            if (string_keys.find(key) != string_keys.end())
                return error(CONFIG_ERR_OPTION_EXISTS, 0, key);
            else
                string_keys.insert({key, &variable});
            
            return error(CONFIG_SUCCESS, 0);
        }

        int parse() {
            std::ifstream file(path);
            std::string line;
            std::vector<std::string> foundkeys;

            int linenumber = 0;

            if (!file.is_open()) 
                return error(CONFIG_ERR_FILE_NOT_FOUND, 0, path);

            while (std::getline(file, line)) {
                linenumber++;

                if (line.find_first_not_of(' ') == line.npos)
                    continue;
                if (line == "\n")
                    continue;
                if (line[0] == '#')
                    continue;

                std::string key;
                std::string val;
                bool foundEqual = false;

                for (auto& i : line) {
                    if (i == '\n')
                        break;
                    
                    if (i == '#')
                        break;

                    if (foundEqual) {
                        val += i;
                        continue;
                    }

                    if (i == '=')
                        foundEqual = true;
                    else
                        key += i;
                }

                if (!foundEqual)
                    return error(CONFIG_ERR_SYNTAX_ERROR, linenumber);

                if (std::find(foundkeys.begin(), foundkeys.end(), key) != foundkeys.end()) {
                    return error(CONFIG_ERR_KEY_EXISTS, linenumber, key);
                }

                if (boolean_keys.count(key) > 0) {
                    if (val == "true")
                        *boolean_keys[key] = true;
                    else if (val == "false")
                        *boolean_keys[key] = false;
                    else
                        return error(CONFIG_ERR_KEY_TYPE_MISMATCH, linenumber, val, "bool");
                } else if (string_keys.count(key) > 0) {
                    *string_keys[key] = val;
                }
                else
                    return error(CONFIG_ERR_INVALID_KEYNAME, linenumber, key);

                foundkeys.push_back(key);
            }

            file.close();

            return error(CONFIG_SUCCESS, 0);
        }
    };
}
