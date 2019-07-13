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

// stolen from https://stackoverflow.com/questions/5043403/listing-only-folders-in-directory
std::vector<std::string> get_directories(const std::string& s){
    std::vector<std::string> r;
    for(auto& p : std::filesystem::recursive_directory_iterator(s)){
        if(p.status().type() == std::filesystem::file_type::directory){
            r.push_back(p.path().string());
        }
    }

    return r;
}

int main(int argc, char *argv[]) {
    using namespace communication;
    if (argc != 6 && argc != 7 && argc != 8 && argc != 9) {
        std::cerr << "Usage: KiTraining matchConfig.json leftTeamConfig.json rightTeamConfig.json learningRate discountRate [pretrainedNet] [experienceDirectory experienceReplayEpochCount]" << std::endl;
        std::exit(1);
    }

    std::string matchConfigPath{argv[1]};
    std::string leftTeamConfiPath{argv[2]};
    std::string rightTeamConfigPath{argv[3]};
    auto learningRate = std::stod(argv[4]);
    auto discountRate = std::stod(argv[5]);
    std::optional<std::vector<std::string>> expDirList;
    std::optional<int> expEpochs;
    std::optional<std::filesystem::directory_iterator> expDirIt;
    std::optional<std::string> pretrainedNet;
    bool expReplayEnabled = false;
    std::vector<std::string>::iterator dirListIt;

    if(argc == 7) {
        pretrainedNet.emplace(argv[6]);
    } else if(argc == 8) {
        expDirList.emplace(get_directories(argv[6]));
        expEpochs.emplace(std::stoi(argv[7]));
    } else if(argc == 9) {
        pretrainedNet.emplace(argv[6]);
        expDirList.emplace(get_directories(argv[7]));
        expEpochs.emplace(std::stoi(argv[8]));
    }

    auto matchConfig = readFromFileToJson<messages::broadcast::MatchConfig>(matchConfigPath);
    auto leftTeamConfig = readFromFileToJson<messages::request::TeamConfig>(leftTeamConfiPath);
    auto rightTeamConfig = readFromFileToJson<messages::request::TeamConfig>(rightTeamConfigPath);

    util::Logging log{std::cout, 4};

    std::unique_ptr<std::pair<ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>,
            ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>>> mlps;

    if(pretrainedNet.has_value()){
        log.info("--- training with pretrained net ---");
        mlps = std::make_unique<decltype(mlps)::element_type>(std::make_pair(ml::util::loadFromFile<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>(*pretrainedNet),
                    ml::util::loadFromFile<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>(*pretrainedNet)));
    } else {
        mlps = std::make_unique<std::pair<ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>,
            ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>>>(std::make_pair<ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>,
            ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>>({ml::functions::relu, ml::functions::relu, ml::functions::identity},
                                                                   {ml::functions::relu, ml::functions::relu, ml::functions::identity}));
    }

    if(argc < 8){
        log.info("No directory for experience replay specified");
    }

    if(expDirList.has_value() && expDirList->empty()){
        log.warn("No experiences found in " + std::string(argv[7]));
    } else {
        log.warn("Found " + std::to_string(expDirList->size()) + " directories in " + std::string(argv[7]));
        expReplayEnabled = true;
        dirListIt = expDirList->begin();
        expDirIt = std::filesystem::directory_iterator(*dirListIt);
    }


    for (auto epoch = 0; epoch < std::numeric_limits<int>::max(); ++epoch) {
        std::unique_ptr<Communicator> communicator;
        if(expReplayEnabled && epoch % *expEpochs == 0){
            if(*expDirIt == std::filesystem::end(*expDirIt)) {
                if(++dirListIt == expDirList->end()){
                    log.warn("--- No experience left, resetting ---");
                    dirListIt = expDirList->begin();
                }

                log.warn("--- Current experience exhausted, fetching next directory ---");
                expDirIt.emplace(std::filesystem::directory_iterator(*dirListIt));
            }

            log.warn("--- Experience replay epoch ---");
            auto state = readFromFileToJson<aiTools::State>(expDirIt.value()->path().string());
            communicator = std::make_unique<Communicator>(matchConfig, state, log, learningRate, discountRate, *mlps, "---");
            ++(*expDirIt);

        } else {
            communicator = std::make_unique<Communicator>(matchConfig, leftTeamConfig, rightTeamConfig, log, learningRate,
                                                 discountRate, *mlps, "---");
        }

        mlps = std::make_unique<decltype(communicator->mlps)>(communicator->mlps);
        log.warn("Epoch finished: " + std::to_string(epoch));

        if (epoch % 10000 == 0) {
            ml::util::saveToFile(std::string{"trainingFiles/left_epoch"} + std::to_string(epoch) + std::string{".json"},
                                 mlps->first);
            ml::util::saveToFile(
                    std::string{"trainingFiles/right_epoch"} + std::to_string(epoch) + std::string{".json"},
                    mlps->second);
        }
    }

    return 0;
}
