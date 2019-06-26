//
// Created by timluchterhand on 06.05.19.
//

#ifndef SERVER_PHASEMANAGER_H
#define SERVER_PHASEMANAGER_H

#include <SopraMessages/types.hpp>
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <SopraMessages/Next.hpp>
#include <queue>
#include "GameTypes.h"
#include "MemberSelector.h"

namespace gameHandling{
    enum class PlayerTurnState{
        Move,
        ExtraMove,
        PossibleAction
    };

    enum class TeamState{
        OneEmpty,
        BothEmpty,
        BothAvailable
    };

    class PhaseManager {
    public:
        PhaseManager(const std::shared_ptr<gameModel::Team> &teamLeft,
                     const std::shared_ptr<gameModel::Team> &teamRight,
                     std::shared_ptr<const gameModel::Environment> env, Timeouts timeouts);

        /**
         * Returns the next Player action required by a client
         * @return Next action to be taken, or nothing if player phase is over
         */
        auto nextPlayer() -> std::optional<communication::messages::broadcast::Next>;

        /**
         * Returns the next Fan action required by a client
         * @return Next action to be taken, or nothing if fan phase is over
         */
        auto nextInterference() -> std::optional<communication::messages::broadcast::Next>;

        /**
         *
         * @return true if at least one Team can use an Interference
         */
        bool hasInterference() const;

        /**
         * Resets the internal states so that players can be selected again
         */
        void resetPlayers();

        /**
         * Resets the internal states so that interferences can be selected again
         */
        void resetInterferences();

        /**
         * Resets all internal states so that players and interferences can be selected again
         */
        void reset();

        /**
         *
         * @param type
         * @return the number of times the specified fan was used (left team)
         */
        int interferencesUsedLeft(communication::messages::types::FanType type) const;

        /**
         *
         * @param type
         * @return the number of times the specified fan was used (right team)
         */
        int interferencesUsedRight(communication::messages::types::FanType type) const;

        /**
         *
         * @param player
         * @return true if the specified player already used their entire turn
         */
        bool playerUsed(const std::shared_ptr<const gameModel::Player> &player) const;

    private:
        MemberSelector teamLeft, teamRight; ///< Member selectors for both teams
        const std::shared_ptr<const gameModel::Environment> env; ///<The immutable environment for rule checks
        gameModel::TeamSide currentSidePlayers, currentSideInter; ///< The current sides for Interferences and Players
        std::optional<std::shared_ptr<gameModel::Player>> currentPlayer; ///<Current Player to make a move
        PlayerTurnState playerTurnState = PlayerTurnState::Move; ///<The state of the current Players turn
        TeamState teamStatePlayers = TeamState::BothAvailable; ///<State of the Memberselectors
        TeamState teamStateInterferences = TeamState::BothAvailable; ///<State of the Memberselectors
        gameHandling::Timeouts timeouts;

        /**
         * Randomly initializes the given Teamside
         * @param side
         */
        void chooseSide(gameModel::TeamSide &side) const;

        /**
         * gets the Memberselector by teamside
         * @param side
         * @return
         */
        auto getTeam(gameModel::TeamSide side) -> MemberSelector&;

        /**
         * gets the Memberselector by teamside
         * @param side
         * @return
         */
        auto getTeam(gameModel::TeamSide side) const -> const MemberSelector&;

        /**
         * Switches the teamside
         * @param side the Team side enum to be toggled
         */
        void switchSide(gameModel::TeamSide &side);
    };
}


#endif //SERVER_PHASEMANAGER_H
