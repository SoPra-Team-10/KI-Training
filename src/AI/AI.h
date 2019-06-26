//
// Created by timluchterhand on 26.06.19.
//

#ifndef SERVER_AI_H
#define SERVER_AI_H

#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/GameController.h>
#include <SopraMessages/types.hpp>
#include <unordered_set>
#include <Mlp/Mlp.hpp>
#include <SopraMessages/Next.hpp>
#include <SopraMessages/DeltaRequest.hpp>

namespace AI{
    constexpr auto FEATURE_VEC_LEN = 122;
    struct State{
        std::shared_ptr<const gameModel::Environment> env;
        unsigned int roundNumber;
        communication::messages::types::PhaseType currentPhase;
        gameController::ExcessLength  overtimeState;
        unsigned int overTimeCounter;
        bool goalScoredThisRound;
        std::unordered_set<communication::messages::types::EntityId> playersUsedLeft;
        std::unordered_set<communication::messages::types::EntityId> playersUsedRight;
        std::array<unsigned int, 5> availableFansLeft; //Teleport, RangedAttack, Impulse, SnitchPush, BlockCell
        std::array<unsigned int, 5> availableFansRight;
    };

    class AI {
    public:
        AI(const std::shared_ptr<gameModel::Environment>& env, gameModel::TeamSide mySide);

        /**
         * Updates the internal State
         * @param state new State
         */
        void update(const State &state);

        /**
         * Returns the AIs next action
         * @param next
         * @return
         */
        auto getNextAction(const communication::messages::broadcast::Next &next) const ->
            std::optional<communication::messages::request::DeltaRequest>;

    private:
        State currentState;
        const gameModel::TeamSide mySide;
        ml::Mlp<FEATURE_VEC_LEN, 200, 200, 1> stateEstimator;

        /**
         * Computes a feature vextor from the current state
         * @return
         */
        auto getFeatureVec() const -> std::array<double, FEATURE_VEC_LEN>;
    };
}



#endif //SERVER_AI_H
