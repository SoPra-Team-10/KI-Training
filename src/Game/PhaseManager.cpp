//
// Created by timluchterhand on 06.05.19.
//

#include "PhaseManager.h"
#include <SopraGameLogic/conversions.h>

namespace gameHandling{
    PhaseManager::PhaseManager(const std::shared_ptr<gameModel::Team> &team1,
             const std::shared_ptr<gameModel::Team> &team2, std::shared_ptr<const gameModel::Environment> env,
             Timeouts timeouts) : team1(team1), team2(team2), env(std::move(env)), timeouts(timeouts){
        chooseSide(currentSidePlayers);
        chooseSide(currentSideInter);
    }

    auto PhaseManager::nextPlayer() -> std::optional<communication::messages::broadcast::Next> {
        using namespace communication::messages;
        bool noTurnAllowed = false;
        if(currentPlayer.has_value() && currentPlayer.value()->isFined){
            playerTurnState = PlayerTurnState::Move;
            currentPlayer.reset();
            return nextPlayer();
        }

        switch (playerTurnState){
            case PlayerTurnState::Move:
                switch (teamStatePlayers){
                    case TeamState::BothAvailable:{
                        auto &team = getTeam(currentSidePlayers);
                        currentPlayer = team.getNextPlayer();
                        if(currentPlayer.value()->knockedOut || currentPlayer.value()->isFined){
                            currentPlayer.value()->knockedOut = false;
                            noTurnAllowed = true;
                        }

                        if(!team.hasPlayers()){
                            teamStatePlayers = TeamState::OneEmpty;
                            switchSide(currentSidePlayers);
                            break;
                        }

                        if(!currentPlayer.value()->isFined){
                            switchSide(currentSidePlayers);
                        }

                        break;
                    }

                    case TeamState::OneEmpty:{
                        auto &team = getTeam(currentSidePlayers);
                        currentPlayer = team.getNextPlayer();
                        if(currentPlayer.value()->knockedOut || currentPlayer.value()->isFined){
                            currentPlayer.value()->knockedOut = false;
                            noTurnAllowed = true;
                        }

                        if(!team.hasPlayers()){
                            teamStatePlayers = TeamState::BothEmpty;
                        }

                        break;
                    }

                    case TeamState::BothEmpty:
                        return {};
                }

                if(noTurnAllowed){
                    return nextPlayer();
                }

                if(gameController::actionTriggered(env->config.getExtraTurnProb(currentPlayer.value()->broom))){
                    playerTurnState = PlayerTurnState::ExtraMove;
                } else {
                    playerTurnState = PlayerTurnState::PossibleAction;
                }

                return broadcast::Next{currentPlayer.value()->getId(), types::TurnType::MOVE, timeouts.playerTurn};
            case PlayerTurnState::ExtraMove:
                playerTurnState = PlayerTurnState::PossibleAction;
                return broadcast::Next{currentPlayer.value()->getId(), types::TurnType::MOVE, timeouts.playerTurn};
            case PlayerTurnState::PossibleAction:
                playerTurnState = PlayerTurnState::Move;
                if(gameController::playerCanPerformAction(currentPlayer.value(), env)){
                    auto id = currentPlayer.value()->getId();
                    currentPlayer.reset();
                    return broadcast::Next{id, types::TurnType::ACTION, timeouts.playerTurn};
                } else {
                    return nextPlayer();
                }

            default:
                throw std::runtime_error("Enum out of bounds");
        }
    }

    auto PhaseManager::nextInterference() -> std::optional<communication::messages::broadcast::Next> {
        using namespace communication::messages;
        types::EntityId nextInter;
        switch (teamStateInterferences){
            case TeamState::BothAvailable:{
                auto &team = getTeam(currentSideInter);
                nextInter = team.getNextInterference();
                if(!team.hasInterference()){
                    teamStateInterferences = TeamState::OneEmpty;
                }

                switchSide(currentSideInter);
                break;
            }

            case TeamState::OneEmpty:{
                auto &team = getTeam(currentSideInter);
                nextInter = team.getNextInterference();
                if(!team.hasInterference()){
                    teamStateInterferences = TeamState::BothEmpty;
                }

                break;
            }

            case TeamState::BothEmpty:
                return {};

            default:
                throw std::runtime_error{"Enum out of bounds"};
        }

        return broadcast::Next{nextInter, types::TurnType::FAN, timeouts.fanTurn};
    }

    void PhaseManager::resetPlayers() {
        team1.resetPlayers();
        team2.resetPlayers();
        chooseSide(currentSidePlayers);
        teamStatePlayers = TeamState::BothAvailable;
    }

    void PhaseManager::resetInterferences() {
        team2.resetInterferences();
        team1.resetInterferences();
        if(team2.hasInterference() && team1.hasInterference()){
            chooseSide(currentSideInter);
            teamStateInterferences = TeamState::BothAvailable;
        } else if(team1.hasInterference()){
            currentSideInter = team1.getSide();
            teamStateInterferences = TeamState::OneEmpty;
        } else if(team2.hasInterference()){
            currentSideInter = team2.getSide();
            teamStateInterferences = TeamState::OneEmpty;
        }else {
            teamStateInterferences = TeamState::BothEmpty;
        }
    }

    void PhaseManager::reset() {
        resetPlayers();
        resetInterferences();
    }


    auto PhaseManager::getTeam(gameModel::TeamSide side) -> MemberSelector & {
        return team1.getSide() == side ? team1 : team2;
    }

    auto PhaseManager::getTeam(gameModel::TeamSide side) const -> const MemberSelector & {
        return team1.getSide() == side ? team1 : team2;
    }

    void PhaseManager::chooseSide(gameModel::TeamSide &side) const{
        if(gameController::rng(0, 1)){
            side = gameModel::TeamSide::LEFT;
        } else {
            side = gameModel::TeamSide::RIGHT;
        }
    }

    int PhaseManager::interferencesUsedLeft(communication::messages::types::FanType type) const {
        return getTeam(gameModel::TeamSide::LEFT).usedInterferences(type);
    }

    int PhaseManager::interferencesUsedRight(communication::messages::types::FanType type) const {
        return getTeam(gameModel::TeamSide::RIGHT).usedInterferences(type);
    }

    void PhaseManager::switchSide(gameModel::TeamSide &side) {
        side = side == gameModel::TeamSide::LEFT ? gameModel::TeamSide::RIGHT : gameModel::TeamSide::LEFT;
    }

    bool PhaseManager::playerUsed(const std::shared_ptr<const gameModel::Player> &player) const {
        auto &team = getTeam(gameLogic::conversions::idToSide(player->getId()));
        return team.playerUsed(player->getId()) && (currentPlayer != player || (playerTurnState == PlayerTurnState::PossibleAction &&
            !gameController::playerCanPerformAction(player, env)));
    }

    bool PhaseManager::hasInterference() const{
        return team1.hasInterference() || team2.hasInterference();
    }
}
