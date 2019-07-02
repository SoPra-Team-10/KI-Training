//
// Created by paulnykiel on 26.06.19.
//

#ifndef KITRAINING_COMMUNICATOR_H
#define KITRAINING_COMMUNICATOR_H

#include <SopraMessages/MatchConfig.hpp>
#include <SopraMessages/TeamConfig.hpp>
#include <Game/Game.h>

namespace communication {
    class Communicator {
    public:
        Communicator(const messages::broadcast::MatchConfig &matchConfig,
                const messages::request::TeamConfig &leftTeamConfig,
                const messages::request::TeamConfig &rightTeamConfig,
                util::Logging &log, double learningRate, double discountRate,
                const std::pair<ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>,
                ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>> &mlps, std::string expDir);

        std::pair<ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>,
            ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>> mlps;


    private:
        gameHandling::Game game;
        std::pair<ai::AI, ai::AI> ais;
        util::Logging &log;
    };
}

#endif //KITRAINING_COMMUNICATOR_H
