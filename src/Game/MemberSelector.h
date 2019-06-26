//
// Created by timluchterhand on 06.05.19.
//

#ifndef SERVER_MEMBERSELECTOR_H
#define SERVER_MEMBERSELECTOR_H

#include <SopraMessages/types.hpp>
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <deque>
#include <SopraGameLogic/Interference.h>
#include "GameTypes.h"

namespace gameHandling{
    class MemberSelector {
    public:
        MemberSelector(const std::shared_ptr<gameModel::Team> &team, gameModel::TeamSide side);

        /**
         *
         * @return the next randomly chosen player id from the team
         * @throws std::runtime_error when all players from team have been selected
         */
        auto getNextPlayer() -> std::shared_ptr<gameModel::Player>;

        /**
         *
         * @return true if at least one Player is available
         */
        bool hasPlayers() const;

        /**
         *
         * @param id
         * @return true if the specified PlayerID has already been selected
         */
        bool playerUsed(communication::messages::types::EntityId id) const;

        /**
         *
         * @return the next randomly chosen interference id from the team
         * @throws std::runtime_error when all interferences from team have been selected
         */
        auto getNextInterference() -> communication::messages::types::EntityId;

        /**
         *
         * @return false if all interferences have been selected, true otherwise
         */
        bool hasInterference() const;

        /**
         *
         * @param type
         * @return the number of times the selected fan has been used
         */
        int usedInterferences(communication::messages::types::FanType type) const;

        /**
         * Restores the initial state of the player list just as after the construction
         * Resets the internal representation of the player list, so that it is possible to randomly choose players again.
         */
        void resetPlayers();

        /**
         * Restores the initial state of the interference list just as after the construction
         * Resets the internal representation of the interference list, so that it is possible to randomly choose interferences again.
         */
        void resetInterferences();

    private:
        std::shared_ptr<const gameModel::Team> team; ///<The immutable Team used to reset the internal state of the MemberSelector
        const gameModel::TeamSide side; ///<The Side the Team belongs to
        std::deque<std::shared_ptr<gameModel::Player>> playersLeft; ///<List with all currently available Players
        std::deque<std::pair<gameModel::InterferenceType, int>> interferencesLeft; ///<List with all currently available Interferences and their respective uses

        /**
         *
         * @tparam T Type of deque
         * @param list
         * @return iterator to a randomly choosen element of the deque
         */
        template <typename T>
        auto selectRandom(std::deque<T> &list) const -> typename std::deque<T>::iterator;
    };


    template<typename T>
    auto MemberSelector::selectRandom(std::deque<T> &list) const -> typename std::deque<T>::iterator {
        int index = gameController::rng(0, static_cast<int>(list.size()) - 1);
        return list.begin() + index;
    }
}

#endif