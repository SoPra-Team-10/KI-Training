//
// Created by paulnykiel on 26.06.19.
//

#include <SopraGameLogic/conversions.h>
#include <SopraAITools/AITools.h>
#include <Mlp/Util.h>
#include "Communicator.h"

communication::Communicator::Communicator(const communication::messages::broadcast::MatchConfig &matchConfig,
                                          const communication::messages::request::TeamConfig &leftTeamConfig,
                                          const communication::messages::request::TeamConfig &rightTeamConfig,
                                          util::Logging &log, double learningRate, double discountRate,
                                          const std::pair<ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>,
                            ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>> &mlps, std::string expDir)
                                          : game{matchConfig, leftTeamConfig, rightTeamConfig,
                                                 aiTools::getTeamFormation(gameModel::TeamSide::LEFT),
                                                 aiTools::getTeamFormation(gameModel::TeamSide::RIGHT), log, std::move(expDir)},
                                            ais{std::make_pair(
                                                    ai::AI{game.environment, gameModel::TeamSide::LEFT, learningRate, discountRate, log},
                                                    ai::AI{game.environment, gameModel::TeamSide::RIGHT, learningRate, discountRate, log})},
                                                    log{log} {
    run(mlps);
}

communication::Communicator::Communicator(const communication::messages::broadcast::MatchConfig &matchConfig,
                                          const aiTools::State &state, util::Logging &log, double learningRate,
                                          double discountRate,
                                          const std::pair<ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>,
                                                  ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>> &mlps,
                                          std::string expDir) : game{matchConfig, state, log, std::move(expDir)},
                                          ais(std::make_pair(ai::AI{game.environment, gameModel::TeamSide::LEFT, learningRate, discountRate, log},
                                                  ai::AI{game.environment, gameModel::TeamSide::RIGHT, learningRate, discountRate, log})), log(log){
    run(mlps);
}

void communication::Communicator::run(const std::pair<ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>,
        ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>> &nets) {
    ais.first.stateEstimator = nets.first;
    ais.second.stateEstimator = nets.second;

    auto next = game.getNextAction();

    while (!game.winEvent.has_value()) {
        std::optional<gameModel::TeamSide> lastTeamSide;

        if (gameLogic::conversions::isBall(next.getEntityId())) {
            game.executeBallDelta(next.getEntityId());
            log.debug("Ball");
        } else {
            auto action1 = ais.first.getNextAction(next);
            auto action2 = ais.second.getNextAction(next);

            if (action1.has_value() && action2.has_value()) {
                throw std::runtime_error("Both players want to perform an action!");
            } else if (action1.has_value()) {
                game.executeDelta(action1.value(), gameModel::TeamSide::LEFT);
                log.debug("Player 1");
                lastTeamSide = gameModel::TeamSide::LEFT;
            } else if (action2.has_value()) {
                game.executeDelta(action2.value(), gameModel::TeamSide::RIGHT);
                log.debug("Player 2");
                lastTeamSide = gameModel::TeamSide::RIGHT;
            } else {
                throw std::runtime_error{"No player wants to perform an action!"};
            }
        }

        next = game.getNextAction();
        ais.first.update(game.getState(), std::nullopt, lastTeamSide);
        ais.second.update(game.getState(), std::nullopt,lastTeamSide);
        game.saveExperience();
    }
    auto winTuple = game.winEvent.value();

    ais.first.update(game.getState(), winTuple.first, winTuple.first);
    ais.second.update(game.getState(), winTuple.first, winTuple.first);

    log.info("Game finished:");
    log.info(messages::types::toString(winTuple.second));

    this->mlps.first = ais.first.stateEstimator;
    this->mlps.second = ais.second.stateEstimator;
}
