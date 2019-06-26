//
// Created by paulnykiel on 26.06.19.
//

#include <SopraGameLogic/conversions.h>
#include "Communicator.h"

communication::Communicator::Communicator(const communication::messages::broadcast::MatchConfig &matchConfig,
                                          const communication::messages::request::TeamConfig &leftTeamConfig,
                                          const communication::messages::request::TeamConfig &rightTeamConfig,
                                          util::Logging &log)
                                          : game{matchConfig, leftTeamConfig, rightTeamConfig, {}, {}/*TODO*/, log},
                                          ais{std::make_pair(ai::AI{game.environment, gameModel::TeamSide::LEFT},
                                             ai::AI{game.environment, gameModel::TeamSide::RIGHT})}, log{log} {

    while (!game.winEvent.has_value()) {
        ais.first.update(game.getState());
        ais.second.update(game.getState());

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

    log.info("Game finished:");
    log.info(messages::types::toString(game.winEvent.value().second));
}