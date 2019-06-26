//
// Created by paulnykiel on 26.06.19.
//

#include <SopraGameLogic/conversions.h>
#include <SopraAITools/AITools.h>
#include "Communicator.h"

communication::Communicator::Communicator(const communication::messages::broadcast::MatchConfig &matchConfig,
                                          const communication::messages::request::TeamConfig &leftTeamConfig,
                                          const communication::messages::request::TeamConfig &rightTeamConfig,
                                          util::Logging &log, double learningRate, double discountRate)
                                          : game{matchConfig, leftTeamConfig, rightTeamConfig,
                                                 aiTools::getTeamFormation(gameModel::TeamSide::LEFT),
                                                 aiTools::getTeamFormation(gameModel::TeamSide::RIGHT), log},
                                            ais{std::make_pair(
                                                    ai::AI{game.environment, gameModel::TeamSide::LEFT, learningRate, discountRate, log},
                                                    ai::AI{game.environment, gameModel::TeamSide::RIGHT, learningRate, discountRate, log})},
                                                    log{log} {

    while (!game.winEvent.has_value()) {
        ais.first.update(game.getState(), std::nullopt);
        ais.second.update(game.getState(), std::nullopt);

        auto next = game.getNextAction();

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
            } else if (action2.has_value()) {
                game.executeDelta(action2.value(), gameModel::TeamSide::RIGHT);
                log.debug("Player 2");
            } else {
                throw std::runtime_error{"No player wants to perform an action!"};
            }
        }
    }
    auto winTuple = game.winEvent.value();

    ais.first.update(game.getState(), winTuple.first);
    ais.second.update(game.getState(), winTuple.first);

    log.info("Game finished:");
    log.info(messages::types::toString(winTuple.second));
}
