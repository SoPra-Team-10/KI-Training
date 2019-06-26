//
// Created by timluchterhand on 26.06.19.
//

#include <SopraGameLogic/conversions.h>
#include "AI.h"
namespace AI{
    AI::AI(const std::shared_ptr<gameModel::Environment>& env, gameModel::TeamSide mySide) :
        currentState{env, 1, communication::messages::types::PhaseType::BALL_PHASE, gameController::ExcessLength::None,
                     0, false, {}, {}, {}, {}}, mySide(mySide), stateEstimator(ml::functions::relu, ml::functions::relu, ml::functions::identity){}

    void AI::update(const State &state) {
        this->currentState = state;
    }

    auto AI::getFeatureVec() const -> std::array<double, FEATURE_VEC_LEN> {
        std::array<double, FEATURE_VEC_LEN> ret = {};
        auto insertTeam = [this](gameModel::TeamSide side, std::array<double, 120>::iterator &it){
            auto &usedPlayers = side == gameModel::TeamSide::LEFT ? currentState.playersUsedLeft : currentState.playersUsedRight;
            auto &availableFans = side == gameModel::TeamSide::LEFT ? currentState.availableFansLeft : currentState.availableFansRight;
            for(const auto &player : currentState.env->getTeam(side)->getAllPlayers()){
                *it++ = player->position.x;
                *it++ = player->position.y;
                bool used = false;
                for(const auto &id : usedPlayers){
                    if(player->id == id){
                        used = true;
                        break;
                    }
                }

                *it++ = used;
                *it++ = !used && !player->knockedOut && !player->isFined;
                *it++ = player->knockedOut;
                *it++ = player->isFined;
            }

            for(const auto &useNumber : availableFans){
                *it++ = useNumber;
            }
        };

        auto opponentSide = mySide == gameModel::TeamSide::LEFT ? gameModel::TeamSide::RIGHT : gameModel::TeamSide::LEFT;
        ret[0] = currentState.roundNumber;
        ret[1] = static_cast<double>(currentState.currentPhase);
        ret[2] = static_cast<double>(currentState.overtimeState);
        ret[3] = currentState.overTimeCounter;
        ret[4] = currentState.goalScoredThisRound;
        ret[5] = currentState.env->getTeam(mySide)->score;
        ret[6] = currentState.env->getTeam(opponentSide)->score;
        auto it = ret.begin() + 7;
        for(const auto &shit : currentState.env->pileOfShit){
            *it++ = shit->position.x;
            *it++ = shit->position.y;
        }

        for(unsigned long i = 0; i < 12 - currentState.env->pileOfShit.size(); i++){
            *it++ = 0;
            *it++ = 0;
        }

        ret[19] = currentState.env->quaffle->position.x;
        ret[20] = currentState.env->quaffle->position.y;
        ret[21] = currentState.env->bludgers[0]->position.x;
        ret[22] = currentState.env->bludgers[0]->position.y;
        ret[23] = currentState.env->bludgers[1]->position.x;
        ret[24] = currentState.env->bludgers[1]->position.y;
        ret[25] = currentState.env->snitch->position.x;
        ret[26] = currentState.env->snitch->position.y;
        ret[27] = currentState.env->snitch->exists;
        it = ret.begin() + 28;
        insertTeam(mySide, it);
        insertTeam(opponentSide, it);
        return ret;
    }

    auto AI::getNextAction(const communication::messages::broadcast::Next &next) const ->
        std::optional<communication::messages::request::DeltaRequest> {

        if(gameLogic::conversions::idToSide(next.getEntityId()) != mySide){
            return std::nullopt;
        }

        switch (next.getTurnType()){
            case communication::messages::types::TurnType::MOVE:
                break;
            case communication::messages::types::TurnType::ACTION:break;
            case communication::messages::types::TurnType::FAN:break;
            case communication::messages::types::TurnType::REMOVE_BAN:break;
        }
    }
}
