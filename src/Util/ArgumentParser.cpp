/**
 * @file ArgumentParser.cpp
 * @author paul
 * @date 01.05.19
 * @brief Definition of the ArgumentParser class
 */

#include "ArgumentParser.hpp"
#include <getopt.h>
#include <iostream>

namespace util {
    ArgumentParser::ArgumentParser(int argc, char **argv) : match{}, port{0}, verbosity{0} {
        if (argc <= 1) {
            printHelp();
            std::exit(1);
        }

        option options[] = {
                {"match", required_argument, nullptr, 'm'},
                {"help", no_argument, nullptr, 'h'},
                {"port", required_argument, nullptr, 'p'},
                {"verbosity", required_argument, nullptr, 'v'}
        };

        int arg;
        int portI = 8080, verbosityI = 0;
        while ((arg = getopt_long(argc, argv, "m:hp:v:", options, nullptr)) != -1) {
            switch (arg) {
                case 'm':
                    match = optarg;
                    break;
                case 'h':
                    printHelp();
                    std::exit(0);
                case 'p':
                    try {
                        portI = std::stoi(optarg);
                    } catch (std::invalid_argument &e) {
                        throw std::invalid_argument{"Port is not a valid integer"};
                    }
                    break;
                case 'v':
                    try {
                        verbosityI = std::stoi(optarg);
                    } catch (std::invalid_argument &e) {
                        throw std::invalid_argument{"Verbosity is not a valid integer"};
                    }
                    break;
                default:
                    std::exit(1);
            }
        }

        if (portI <= 0 || portI >= 65536) {
            throw std::invalid_argument{"Port is not a valid 16-bit number"};
        }

        if (verbosityI < 0) {
            throw std::invalid_argument{"Verbosity needs to be > 0"};
        }

        this->port = static_cast<uint16_t>(portI);
        this->verbosity = static_cast<unsigned int>(verbosityI);
    }

    auto ArgumentParser::getMatch() const -> std::string {
        return this->match;
    }

    auto ArgumentParser::getPort() const -> uint16_t {
        return this->port;
    }

    auto ArgumentParser::getVerbosity() const -> unsigned int {
        return this->verbosity;
    }

    void ArgumentParser::printHelp() {
        std::cout << "Mandatory Options:\n"
                  << "\t-m --match <path> Pfad zur Partie-Konfiguration\n"
                  << "Optional Options:\n"
                  << "\t -h --help Hilfe\n"
                  << "\t -p --port <port> Server-Port\n"
                  << "\t -v --verbosity <int> Verbosity (ausführlichkeit der Log-Information)" << std::endl;
    }
}
