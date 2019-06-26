/**
 * @file ArgumentParser.hpp
 * @author paul
 * @date 01.05.19
 * @brief Declaration of the ArgumentParser class
 */

#ifndef SERVER_ARGUMENTPARSER_HPP
#define SERVER_ARGUMENTPARSER_HPP

#include <string>
#include <optional>

namespace util {
    /**
     * Implements a basic argument parser according to the standard using GNU-Getopt
     */
    class ArgumentParser {
    public:
        /**
         * CTor, parses the arguments and fills all fields. If --help is found printHelp() is called and the
         * program gets terminated. If no argument is passed the help is printed as well.
         * @throws std::runtime_error if any of the required flags doesn't exist or contains an invalid value
         * @param argc the number of arguments, as passed into main
         * @param argv an array of all arguments, as passed into main
         */
        ArgumentParser(int argc, char *argv[]);

        /**
         * Get the match-config file name as passed by -m or --match. It is guaranteed that this is a valid string
         * but not that the file actually exists or is valid.
         * @return a string containing the argument passed with -m or --match
         */
        auto getMatch() const -> std::string;

        /**
         * Returns the port if provided by -p or --port, else returns the default port of 8080.
         * @return a uint16_t containing the port
         */
        auto getPort() const -> uint16_t;

        /**
         * Returns the verbosity if provided by -v or --verbosity, else the default verbosity of 0
         * @return a unsigned int containing the verbosity
         */
        auto getVerbosity() const -> unsigned int;

        /**
         * Prints the help message, gets called by the CTor if the -h or --help flag is set.
         */
        static void printHelp();
    private:
        std::string match;
        uint16_t port;
        unsigned int verbosity;
    };
}

#endif //SERVER_ARGUMENTPARSER_HPP
