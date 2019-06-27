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
                                          util::Logging &log, double learningRate, double discountRate, int epoch)
                                          : game{matchConfig, leftTeamConfig, rightTeamConfig,
                                                 aiTools::getTeamFormation(gameModel::TeamSide::LEFT),
                                                 aiTools::getTeamFormation(gameModel::TeamSide::RIGHT), log},
                                            ais{std::make_pair(
                                                    ai::AI{game.environment, gameModel::TeamSide::LEFT, learningRate, discountRate, log},
                                                    ai::AI{game.environment, gameModel::TeamSide::RIGHT, learningRate, discountRate, log})},
                                                    log{log} {
    if (epoch > 0) {
        ais.first.stateEstimator = ml::util::loadFromFile<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>
                (std::string{"trainingFiles/left_epoch"} + std::to_string(epoch-1) + std::string{".json"});
        ais.second.stateEstimator = ml::util::loadFromFile<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1>
                (std::string{"trainingFiles/right_epoch"} + std::to_string(epoch-1) + std::string{".json"});
    }

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
    }
    auto winTuple = game.winEvent.value();

    ais.first.update(game.getState(), winTuple.first, winTuple.first);
    ais.second.update(game.getState(), winTuple.first, winTuple.first);

    log.info("Game finished:");
    log.info(messages::types::toString(winTuple.second));

    ml::util::saveToFile(std::string{"trainingFiles/left_epoch"} + std::to_string(epoch) + std::string{".json"},
            ais.first.stateEstimator);
    ml::util::saveToFile(std::string{"trainingFiles/right_epoch"} + std::to_string(epoch) + std::string{".json"},
            ais.second.stateEstimator);
}
