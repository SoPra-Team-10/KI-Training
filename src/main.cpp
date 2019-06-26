#include <getopt.h>
#include <iostream>
#include <filesystem>
#include <fstream>

#include <Communication/Communicator.hpp>
#include "../Util/ArgumentParser.hpp"
#include <SopraUtil/Logging.hpp>

int main(int argc, char *argv[]) {
    std::string match;
    uint16_t port;
    unsigned int verbosity;
    try {
        util::ArgumentParser argumentParser{argc, argv};
        match = argumentParser.getMatch();
        port = argumentParser.getPort();
        verbosity = argumentParser.getVerbosity();
    } catch (std::invalid_argument &e) {
        std::cerr << e.what() << std::endl;
        std::exit(1);
    }

    if(!std::filesystem::exists(match)) {
        std::cerr << "json file doesn't exist!" << std::endl;
        std::exit(1);
    }

    std::ifstream ifstream{match};
    nlohmann::json j;
    communication::messages::broadcast::MatchConfig matchConfig{};
    try {
        ifstream >> j;
    } catch (nlohmann::json::exception &e) {
        std::cerr << "Not a valid json file: " << e.what() << std::endl;
        std::exit(1);
    }

    try {
        matchConfig = j.get<communication::messages::broadcast::MatchConfig>();
    } catch (nlohmann::json::exception &e) {
        std::cerr << "Json is not a valid matchConfig: " << e.what() << std::endl;
        std::exit(1);
    } catch (std::runtime_error &e) {
        std::cerr << "Error with json value according to standard: " << e.what() << std::endl;
        std::exit(1);
    }


    util::Logging log{std::cout, verbosity};

    communication::MessageHandler messageHandler{port, log};
    communication::Communicator communicator{messageHandler, log, matchConfig};

    log.info("Started server");

    while (true) {
        std::this_thread::sleep_for(std::chrono::hours{100});
    }
}
