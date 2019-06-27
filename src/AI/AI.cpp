//
// Created by timluchterhand on 26.06.19.
//

#include <SopraGameLogic/conversions.h>
#include "AI.h"
namespace ai{
    constexpr auto winReward = 1;
    constexpr auto goalReward = 0.2;
    AI::AI(const std::shared_ptr<gameModel::Environment> &env, gameModel::TeamSide mySide, double learningRate,
           double discountRate, util::Logging log) : stateEstimator(ml::functions::relu, ml::functions::relu, ml::functions::identity),
           currentState{env, 1, communication::messages::types::PhaseType::BALL_PHASE, gameController::ExcessLength::None,
                     0, false, {}, {}, {}, {}}, mySide(mySide),
                                                     learningRate(learningRate), discountRate(discountRate), log(log) {}

    void AI::update(const aiTools::State &state, const std::optional<gameModel::TeamSide> &winningSide) {
        double reward = 0;
        auto opponentSide = mySide == gameModel::TeamSide::LEFT ? gameModel::TeamSide::RIGHT : gameModel::TeamSide::LEFT;
        if(winningSide.has_value()){
            if(winningSide != mySide){
                reward = -winReward;
            } else {
                reward = winReward;
            }
        } else {
            auto myDiff = state.env->getTeam(mySide)->score - currentState.env->getTeam(mySide)->score;
            auto opponentDiff = state.env->getTeam(opponentSide)->score - currentState.env->getTeam(opponentSide)->score;
            if(myDiff >= gameController::GOAL_POINTS){
                reward = goalReward;
            } else if(opponentDiff >= gameController::GOAL_POINTS){
                reward = -goalReward;
            }
        }

        auto tdErrorFun = [&reward, &state, this](const std::array<double, 1> &out, const std::array<double, 1> &){
            auto tdError = reward + discountRate * stateEstimator.forward(state.getFeatureVec(mySide))[0] - out[0];
            log.debug(std::string("tdError: ") + std::to_string(tdError));
            return tdError;
        };

        auto stringSide = mySide == gameModel::TeamSide::LEFT ? "left: " : "right: ";
        auto loss = stateEstimator.train({currentState.getFeatureVec(mySide)}, {{0}}, std::numeric_limits<double>::infinity(), tdErrorFun, learningRate);
        log.info(std::string("Loss ") + stringSide + std::to_string(loss));
        this->currentState = state;
    }

    auto AI::getFeatureVec(const aiTools::State &state) const -> std::array<double, aiTools::State::FEATURE_VEC_LEN> {
        std::array<double, aiTools::State::FEATURE_VEC_LEN> ret = {};
        auto insertTeam = [&state](gameModel::TeamSide side, std::array<double, 120>::iterator &it){
            auto &usedPlayers = side == gameModel::TeamSide::LEFT ? state.playersUsedLeft : state.playersUsedRight;
            auto &availableFans = side == gameModel::TeamSide::LEFT ? state.availableFansLeft : state.availableFansRight;
            for(const auto &player : state.env->getTeam(side)->getAllPlayers()){
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
        ret[0] = state.roundNumber;
        ret[1] = static_cast<double>(state.currentPhase);
        ret[2] = static_cast<double>(state.overtimeState);
        ret[3] = state.overTimeCounter;
        ret[4] = state.goalScoredThisRound;
        ret[5] = state.env->getTeam(mySide)->score;
        ret[6] = state.env->getTeam(opponentSide)->score;
        auto it = ret.begin() + 7;
        for(const auto &shit : state.env->pileOfShit){
            *it++ = shit->position.x;
            *it++ = shit->position.y;
        }

        for(unsigned long i = 0; i < 12 - state.env->pileOfShit.size(); i++){
            *it++ = 0;
            *it++ = 0;
        }

        ret[19] = state.env->quaffle->position.x;
        ret[20] = state.env->quaffle->position.y;
        ret[21] = state.env->bludgers[0]->position.x;
        ret[22] = state.env->bludgers[0]->position.y;
        ret[23] = state.env->bludgers[1]->position.x;
        ret[24] = state.env->bludgers[1]->position.y;
        ret[25] = state.env->snitch->position.x;
        ret[26] = state.env->snitch->position.y;
        ret[27] = state.env->snitch->exists;
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

        auto evalFun = [this](const aiTools::State &state){
            return stateEstimator.forward(getFeatureVec(state))[0];
        };

        switch (next.getTurnType()){
            case communication::messages::types::TurnType::MOVE:
                log.info("Move requested");
                return aiTools::computeBestMove(currentState, evalFun, next.getEntityId());
            case communication::messages::types::TurnType::ACTION:{
                auto type = gameController::getPossibleBallActionType(currentState.env->getPlayerById(next.getEntityId()), currentState.env);
                if(!type.has_value()){
                    throw std::runtime_error("No action possible");
                }

                if(*type == gameController::ActionType::Throw) {
                    log.info("Throw requested");
                    return aiTools::computeBestShot(currentState, evalFun, next.getEntityId());
                } else if(*type == gameController::ActionType::Wrest) {
                    log.info("Wrest requested");
                    return aiTools::computeBestWrest(currentState, evalFun, next.getEntityId());
                } else {
                    throw std::runtime_error("Unexpected action type");
                }
            }
            case communication::messages::types::TurnType::FAN:
                return aiTools::getNextFanTurn(currentState, next);
            case communication::messages::types::TurnType::REMOVE_BAN:
                log.info("Unban requested");
                return aiTools::redeployPlayer(currentState, evalFun, next.getEntityId());
            default:
                throw std::runtime_error("Enum out of bounds");
        }
    }
}
