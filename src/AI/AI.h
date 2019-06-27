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
#include <SopraAITools/AITools.h>
#include <SopraUtil/Logging.hpp>

namespace ai {
    class AI {
    public:
        AI(const std::shared_ptr<gameModel::Environment> &env, gameModel::TeamSide mySide, double learningRate,
           double discountRate, util::Logging log);

        /**
         * Updates the internal State
         * @param state new State
         */
        void update(const aiTools::State &state, const std::optional<gameModel::TeamSide> &winningSide,
                const std::optional<gameModel::TeamSide> &side);

        /**
         * Returns the AIs next action
         * @param next
         * @return
         */
        auto getNextAction(const communication::messages::broadcast::Next &next) const ->
            std::optional<communication::messages::request::DeltaRequest>;

        ml::Mlp<aiTools::State::FEATURE_VEC_LEN, 200, 200, 1> stateEstimator;
    private:
        aiTools::State currentState;
        const gameModel::TeamSide mySide;
        double learningRate;
        double discountRate;
        mutable util::Logging log;

        /**
         * Computes a feature vextor from the given state
         * @return
         */
        auto getFeatureVec(const aiTools::State &state) const -> std::array<double, aiTools::State::FEATURE_VEC_LEN>;
    };
}



#endif //SERVER_AI_H
