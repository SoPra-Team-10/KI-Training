//
// Created by paulnykiel on 26.06.19.
//

#include "Communicator.h"

communication::Communicator::Communicator(const communication::messages::broadcast::MatchConfig &matchConfig,
                                          const communication::messages::request::TeamConfig &leftTeamConfig,
                                          const communication::messages::request::TeamConfig &rightTeamConfig,
                                          util::Logging &log)
                                          : game{matchConfig, leftTeamConfig, rightTeamConfig, {}, {}/*TODO*/, log},
                                          ais{std::make_pair(ai::AI{game.environment, gameModel::TeamSide::LEFT},
                                             ai::AI{game.environment, gameModel::TeamSide::RIGHT})}, log{log} {

}
