//
// Created by timluchterhand on 06.05.19.
//

#ifndef SERVER_GAMETYPES_H
#define SERVER_GAMETYPES_H

#include <SopraMessages/types.hpp>

namespace gameHandling {
    enum class GameState {
        BallPhase,
        PlayerPhase,
        InterferencePhase
    };

    struct Timeouts{
        const int playerTurn, fanTurn, unbanTurn;
    };
}

#endif //SERVER_GAMETYPES_H
