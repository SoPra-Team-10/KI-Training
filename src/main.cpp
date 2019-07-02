#include <iostream>
#include <filesystem>
#include <fstream>

#include <SopraMessages/TeamConfig.hpp>
#include <SopraMessages/MatchConfig.hpp>
#include <SopraUtil/Logging.hpp>
#include <Communication/Communicator.h>
#include <Mlp/Util.h>

template <typename T>
auto readFromFileToJson(const std::string &fname) -> T {
    if (!std::filesystem::exists(fname)) {
        std::cerr << "File \"" << fname << "\" doesn't exist" << std::endl;
        std::exit(1);
    }

    T t{};
    try {
        nlohmann::json json;
        std::ifstream ifstream{fname};
        ifstream >> json;
        t = json.get<T>();
    } catch (nlohmann::json::exception &e) {
        std::cerr << e.what() << std::endl;
        std::exit(1);
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        std::exit(1);
    }

    return t;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: KiTraining matchConfig.json leftTeamConfig.json rightTeamConfig.json learningRate discountRate" << std::endl;
        std::exit(1);
    }

    std::string matchConfigPath{argv[1]};
    std::string leftTeamConfiPath{argv[2]};
    std::string rightTeamConfigPath{argv[3]};
    auto learningRate = std::stod(argv[4]);
    auto discountRate = std::stod(argv[5]);


    auto matchConfig = readFromFileToJson<communication::messages::broadcast::MatchConfig>(matchConfigPath);
    auto leftTeamConfig = readFromFileToJson<communication::messages::request::TeamConfig>(leftTeamConfiPath);
    auto rightTeamConfig = readFromFileToJson<communication::messages::request::TeamConfig>(rightTeamConfigPath);

    util::Logging log{std::cout, 4};

    auto mlps = std::make_pair<ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>,
            ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>>({ml::functions::relu, ml::functions::relu, ml::functions::identity},
                                                                   {ml::functions::relu, ml::functions::relu, ml::functions::identity});

    for (auto epoch = 0; epoch < std::numeric_limits<int>::max(); ++epoch) {
        communication::Communicator communicator{matchConfig, leftTeamConfig, rightTeamConfig, log, learningRate,
                                                 discountRate, mlps, "Experiences/"};

        mlps = communicator.mlps;

        log.warn("Epoch finished: " + std::to_string(epoch));

        if (epoch % 100000 == 0) {
            ml::util::saveToFile(std::string{"trainingFiles/left_epoch"} + std::to_string(epoch) + std::string{".json"},
                                 mlps.first);
            ml::util::saveToFile(
                    std::string{"trainingFiles/right_epoch"} + std::to_string(epoch) + std::string{".json"},
                    mlps.second);
        }
    }

    return 0;
}
