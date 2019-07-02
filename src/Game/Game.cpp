//
// Created by bjorn on 02.05.19.
//

#include "Game.h"
#include <SopraGameLogic/GameController.h>
#include <SopraGameLogic/Interference.h>
#include <SopraGameLogic/GameModel.h>
#include <SopraGameLogic/conversions.h>
#include <AI/AI.h>
#include <fstream>

namespace gameHandling{
    Game::Game(communication::messages::broadcast::MatchConfig matchConfig, const communication::messages::request::TeamConfig& teamConfig1,
            const communication::messages::request::TeamConfig& teamConfig2, communication::messages::request::TeamFormation teamFormation1,
               communication::messages::request::TeamFormation teamFormation2, util::Logging &log, std::string expDir) : environment(std::make_shared<gameModel::Environment>
                       (matchConfig, teamConfig1, teamConfig2, teamFormation1, teamFormation2)),
                       timeouts{matchConfig.getPlayerTurnTimeout(), matchConfig.getFanTurnTimeout(), matchConfig.getUnbanTurnTimeout()},
                       phaseManager(environment->team1, environment->team2, environment, timeouts), log(log), experienceDirectory(std::move(expDir)){
        log.debug("Constructed game");
    }

    Game::Game(communication::messages::broadcast::MatchConfig matchConfig, const aiTools::State &state, util::Logging &log, std::string expDir) :
        environment(state.env), currentPhase(state.currentPhase), roundNumber(state.roundNumber),
        timeouts{matchConfig.getPlayerTurnTimeout(), matchConfig.getFanTurnTimeout(), matchConfig.getUnbanTurnTimeout()},
        phaseManager(environment->team1, environment->team2, environment, timeouts), overTimeState(state.overtimeState),
        overTimeCounter(state.overTimeCounter), goalScored(state.goalScoredThisRound), log(log),experienceDirectory(std::move(expDir)){
        for(const auto &player : environment->getAllPlayers()){
            if(player->isFined){
                bannedPlayers.emplace_back(player);
            }
        }
    }



    auto Game::getNextAction() -> communication::messages::broadcast::Next {
        using namespace communication::messages::types;
        switch (currentPhase){
            case PhaseType::BALL_PHASE:
                switch (ballTurn){
                    case EntityId ::SNITCH :
                        //Bludger1 turn next
                        ballTurn = EntityId::BLUDGER1;

                        //Snitch has to make move if it exists
                        if(environment->snitch->exists){
                            log.debug("Snitch requested to make a move");
                            return {EntityId::SNITCH, TurnType::MOVE, 0};
                        } else {
                            log.debug("Snitch does not exists. Fetching next turn");
                            return getNextAction();
                        }
                    case EntityId::BLUDGER1 :
                        //Bludger2 turn next
                        ballTurn = EntityId::BLUDGER2;
                        log.debug("Bludger1 requested to make a move");
                        return {EntityId::BLUDGER1, TurnType::MOVE, 0};
                    case EntityId ::BLUDGER2 :
                        //Snitch turn next time entering ball phase
                        ballTurn = EntityId::SNITCH;
                        //Ball phase end, Player phase next
                        currentPhase = PhaseType::PLAYER_PHASE;
                        log.debug("Bludger2 requested to make a move");
                        changePhase();
                        return {EntityId::BLUDGER2, TurnType::MOVE, 0};
                    default:
                        throw std::runtime_error("Fatal Error! Inconsistent game state!");
                }

            case PhaseType::PLAYER_PHASE:
                try{
                    auto next = phaseManager.nextPlayer();
                    if(next.has_value()){
                        currentSide = gameLogic::conversions::idToSide(next.value().getEntityId());
                        log.debug("Requested player turn");
                        return expectedRequestType = next.value();
                    } else {
                        currentPhase = PhaseType::FAN_PHASE;
                        changePhase();
                        return getNextAction();
                    }
                } catch (std::exception &e){
                    throw std::runtime_error(e.what());
                }
            case PhaseType::FAN_PHASE:
                try {
                    auto next = phaseManager.nextInterference();
                    if(next.has_value()){
                        currentSide = gameLogic::conversions::idToSide(next.value().getEntityId());
                        log.debug("Requested fan turn");
                        return expectedRequestType = next.value();
                    } else {
                        changePhase();
                        if(goalScored && !bannedPlayers.empty()){
                            currentPhase = PhaseType::UNBAN_PHASE;
                        } else {
                            currentPhase = PhaseType::BALL_PHASE;
                            endRound();
                        }

                        return getNextAction();
                    }
                } catch (std::exception &e){
                    throw std::runtime_error(e.what());
                }
            case PhaseType::UNBAN_PHASE:
                try {
                    if(bannedPlayers.empty()){
                        currentPhase = PhaseType::BALL_PHASE;
                        changePhase();
                        endRound();
                        return getNextAction();
                    } else {
                        log.debug("Requested unban");
                        auto actorId = (*bannedPlayers.begin())->getId();
                        currentSide = gameLogic::conversions::idToSide(actorId);
                        bannedPlayers.erase(bannedPlayers.begin());
                        return expectedRequestType = {actorId, TurnType::REMOVE_BAN, timeouts.unbanTurn};
                    }
                } catch (std::exception &e){
                    throw std::runtime_error(e.what());
                }
            default:
                throw std::runtime_error("Fatal error, inconsistent game state!");
        }
    }

    bool Game::executeDelta(communication::messages::request::DeltaRequest command, gameModel::TeamSide side) {
        using namespace communication::messages::types;
        auto addFouls = [this](const std::vector<gameModel::Foul> &fouls, const std::shared_ptr<gameModel::Player> &player){
            if(!fouls.empty()){
                log.debug("Foul was detected, player banned");
                bannedPlayers.emplace_back(player);
                if(!firstSideDisqualified.has_value() &&
                   environment->getTeam(player)->numberOfBannedMembers() > MAX_BAN_COUNT) {
                    firstSideDisqualified = environment->getTeam(player)->getSide();
                }
            }
        };

        //Request in wrong phase or request from wrong side
        if((currentPhase != PhaseType::PLAYER_PHASE && currentPhase != PhaseType::FAN_PHASE &&
            currentPhase != PhaseType::UNBAN_PHASE) || currentSide != side){
            log.warn("Received request not allowed: Wrong Player or wrong phase");
            return false;
        }

        switch (command.getDeltaType()){
            case DeltaType::SNITCH_CATCH:
                log.warn("Illegal delta request type");
                return false;
            case DeltaType::BLUDGER_BEATING:{
                if(command.getXPosNew().has_value() && command.getYPosNew().has_value() &&
                   command.getActiveEntity().has_value() && command.getPassiveEntity().has_value()){
                    if(!gameLogic::conversions::isPlayer(command.getActiveEntity().value())  ||
                       !gameLogic::conversions::isBall(command.getPassiveEntity().value())){
                        log.warn("Invalid entities for bludger shot");
                        return false;
                    }

                    //Requested different actor or requested move instead of action
                    if(command.getActiveEntity().value() != expectedRequestType.getEntityId() ||
                        expectedRequestType.getTurnType() != TurnType::ACTION){
                        log.warn("Received request not allowed: Wrong entity or no action allowed");
                        return false;
                    }

                    try{
                        auto player = environment->getPlayerById(command.getActiveEntity().value());
                        gameModel::Position target(command.getXPosNew().value(), command.getYPosNew().value());
                        auto targetPlayer = environment->getPlayer(target);
                        auto bludger = environment->getBallByID(command.getPassiveEntity().value());
                        gameController::Shot bShot(environment, player, bludger, target);
                        if(bShot.check() == gameController::ActionCheckResult::Impossible){
                            log.warn("Bludger shot impossible");
                            return false;
                        }

                        auto res = bShot.execute();
                        addFouls(res.second, player);
                        getUsedPlayers(side).emplace(player->getId());
                        return true;
                    } catch (std::exception &e){
                        throw std::runtime_error(e.what());
                    }
                } else {
                    log.warn("Bludger shot has insufficient information");
                    return false;
                }
            }
            case DeltaType::QUAFFLE_THROW:{
                if(command.getActiveEntity().has_value() && command.getXPosNew().has_value() &&
                command.getYPosNew().has_value()){
                    if(!gameLogic::conversions::isPlayer(command.getActiveEntity().value())){
                        log.warn("Invalid entity for quaffle throw");
                        return false;
                    }

                    //Requested different actor or requested move instead of action
                    if(command.getActiveEntity().value() != expectedRequestType.getEntityId() ||
                        expectedRequestType.getTurnType() != TurnType::ACTION){
                        log.debug("Received request not allowed: Wrong entity or no action allowed");
                        return false;
                    }

                    try{
                        auto player = environment->getPlayerById(command.getActiveEntity().value());
                        gameModel::Position target(command.getXPosNew().value(), command.getYPosNew().value());
                        gameController::Shot qThrow(environment, player, environment->quaffle, target);
                        if(qThrow.check() == gameController::ActionCheckResult::Impossible){
                            log.warn("Quaffle throw impossible");
                            return false;
                        }

                        auto res = qThrow.execute();
                        addFouls(res.second, player);
                        for(const auto &result : res.first){
                            if(result == gameController::ActionResult::ScoreLeft ||
                                result == gameController::ActionResult::ScoreRight){
                                goalScored = true;
                                //Notify: goal was scored
                                log.debug("Goal was scored");
                            }
                        }

                        getUsedPlayers(side).emplace(player->getId());
                        return true;
                    } catch (std::exception &e){
                        throw std::runtime_error(e.what());
                    }
                } else{
                    log.warn("Quaffle throw has insufficient information");
                    return false;
                }
            }
            case DeltaType::SNITCH_SNATCH:{
                if(expectedRequestType.getTurnType() != TurnType::FAN){
                    log.warn("Interference request but not in fan phase");
                    return false;
                }

                try{
                    auto team = environment->getTeam(side);
                    gameController::SnitchPush sPush(environment, team);
                    if(!sPush.isPossible()){
                        log.warn("Snitch push is impossible");
                        return false;
                    }

                    if(overTimeState != gameController::ExcessLength::None){
                        log.debug("Overtime, snitch push has no effect");
                        return true;
                    }

                    return true;
                } catch (std::exception &e){
                    throw std::runtime_error(e.what());
                }
            }
            case DeltaType::TROLL_ROAR:{
                if(expectedRequestType.getTurnType() != TurnType::FAN){
                    log.warn("Interference request but not in fan phase");
                    return false;
                }

                try{
                    auto team = environment->getTeam(side);
                    gameController::Impulse impulse(environment, team);
                    if(!impulse.isPossible()){
                        log.warn("Impulse is impossible");
                        return false;
                    }

                    impulse.execute();
                    return true;
                } catch (std::exception &e){
                    throw std::runtime_error(e.what());
                }
            }
            case DeltaType::ELF_TELEPORTATION:{
                if(expectedRequestType.getTurnType() != TurnType::FAN){
                    log.warn("Interference request but not in fan phase");
                    return false;
                }

                if(command.getPassiveEntity().has_value()){
                    if(!gameLogic::conversions::isPlayer(command.getPassiveEntity().value())){
                        log.warn("Teleport target is no player");
                        return false;
                    }

                    try{
                        auto team = environment->getTeam(side);
                        auto targetPlayer = environment->getPlayerById(command.getPassiveEntity().value());
                        gameController::Teleport teleport(environment, team, targetPlayer);
                        if(!teleport.isPossible()){
                            log.warn("Teleport is impossible");
                            return false;
                        }

                        teleport.execute();
                        return true;
                    } catch (std::exception &e){
                        throw std::runtime_error(e.what());
                    }
                } else {
                    log.warn("Teleport request has insufficient information");
                    return false;
                }
            }
            case DeltaType::GOBLIN_SHOCK:
                if(expectedRequestType.getTurnType() != TurnType::FAN){
                    log.warn("Interference request but not in fan phase");
                    return false;
                }

                if(command.getPassiveEntity().has_value()){
                    if(!gameLogic::conversions::isPlayer(command.getPassiveEntity().value())){
                        return false;
                    }

                    try{
                        auto team = environment->getTeam(side);
                        auto targetPlayer = environment->getPlayerById(command.getPassiveEntity().value());
                        gameController::RangedAttack rAttack(environment, team, targetPlayer);
                        if(!rAttack.isPossible()){
                            log.warn("Ranged attack is impossible");
                            return false;
                        }

                        rAttack.execute();
                        return true;
                    } catch (std::exception &e){
                        throw std::runtime_error(e.what());
                    }
                } else {
                    log.warn("Ranged attack request has insufficient information");
                    return false;
                }
            case DeltaType::WOMBAT_POO:
                if(command.getXPosNew().has_value() && command.getYPosNew().has_value()){
                    try{
                        auto shit = gameController::BlockCell(environment, environment->getTeam(side),
                                                              {command.getXPosNew().value(), command.getYPosNew().value()});
                        if(!shit.isPossible()){
                            log.warn(std::string{"BlockCell is impossible"});
                            return false;
                        }

                        shit.execute();
                        return true;
                    } catch (std::exception &e){
                        throw std::runtime_error(e.what());
                    }

                } else {
                    log.warn("Wombat poo request has insufficient information");
                    return false;
                }
            case DeltaType::MOVE:
                if(command.getActiveEntity().has_value() && command.getXPosNew().has_value() &&
                    command.getYPosNew().has_value()){
                    if(!gameLogic::conversions::isPlayer(command.getActiveEntity().value())){
                        log.warn("Moving entity is no player");
                        return false;
                    }

                    if(command.getActiveEntity().value() != expectedRequestType.getEntityId() ||
                        expectedRequestType.getTurnType() != TurnType::MOVE){
                        log.warn("Received request not allowed: Wrong entity or no action allowed");
                        return false;
                    }

                    try{
                        auto player = environment->getPlayerById(command.getActiveEntity().value());
                        gameModel::Position target(command.getXPosNew().value(), command.getYPosNew().value());
                        auto targetPlayer = environment->getPlayer(target);
                        gameController::Move move(environment, player, target);
                        if(move.check() == gameController::ActionCheckResult::Impossible){
                            log.warn("Move is impossible");
                            return false;
                        }

                        auto res = move.execute();
                        addFouls(res.second, player);
                        bool snitchCaught = false;
                        for(const auto &result : res.first){
                            if(result == gameController::ActionResult::ScoreRight ||
                                result == gameController::ActionResult::ScoreLeft){
                                goalScored = true;
                                log.debug("Goal was scored");
                            } else if(result == gameController::ActionResult::SnitchCatch){
                                snitchCaught = true;
                            } else if(result == gameController::ActionResult::FoolAway) {
                                log.debug("Quaffle was lost due to ramming");
                            } else {
                                throw std::runtime_error(std::string{"Unexpected action result"});
                            }
                        }

                        if(environment->snitch->position == player->position && (std::dynamic_pointer_cast<gameModel::Seeker>(player))){
                            if(overTimeState != gameController::ExcessLength::None &&!snitchCaught){
                                snitchCaught = true;
                                environment->getTeam(side)->score += gameController::SNITCH_POINTS;
                            }

                            if(!snitchCaught){
                                log.debug("Failed to catch snitch");
                            }
                        }

                        if(phaseManager.playerUsed(player)){
                            getUsedPlayers(side).emplace(player->getId());
                        }

                        if(snitchCaught){
                            log.debug("Snitch was caught");
                            auto winningTeam = getVictoriousTeam(player);
                            winEvent.emplace(winningTeam.first, winningTeam.second);
                        }

                        return true;
                    } catch (std::exception &e) {
                        throw std::runtime_error(e.what());
                    }
                } else {
                    log.warn("Move request has insufficient information");
                    return false;
                }
            case DeltaType::SKIP:
                if(command.getActiveEntity().has_value()){
                    if(command.getActiveEntity() != expectedRequestType.getEntityId()){
                        log.warn("Received request not allowed: Wrong entity or no action allowed");
                        return false;
                    }

                    if(currentPhase == PhaseType::UNBAN_PHASE) {
                        //Unban skipped -> place on random cell
                        auto player = environment->getPlayerById(command.getActiveEntity().value());
                        environment->placePlayerOnRandomFreeCell(player);
                        player->isFined = false;
                    }

                    return true;
                } else {
                    log.warn("Skip request has insufficient information");
                    return false;
                }
            case DeltaType::UNBAN:
                if(command.getActiveEntity().has_value() && command.getXPosNew().has_value() &&
                    command.getYPosNew().has_value()){
                    if(!gameLogic::conversions::isPlayer(command.getActiveEntity().value())){
                        log.warn("Unban entity is no player");
                        return false;
                    }

                    if(command.getActiveEntity() != expectedRequestType.getEntityId()){
                        log.warn("Received request not allowed: Wrong entity or no action allowed");
                        return false;
                    }

                    try {
                        auto player = environment->getPlayerById(command.getActiveEntity().value());
                        if(!player->isFined){
                            log.warn("Player is not banned");
                            return false;
                        }

                        gameModel::Position target{command.getXPosNew().value(), command.getYPosNew().value()};
                        if(!environment->cellIsFree(target)){
                            log.warn("Invalid target for unban! Cell is occupied");
                            return false;
                        }

                        if(gameModel::Environment::isGoalCell(target)) {
                            log.warn("Invalid target for unban! Must not place player on goal");
                            return false;
                        }

                        log.debug("Unban");
                        player->position = target;
                        player->isFined = false;
                        return true;
                    } catch (std::exception &e) {
                        throw std::runtime_error(e.what());
                    }

                } else {
                    log.warn("Unban request has insufficient information");
                    return false;
                }
            case DeltaType::WREST_QUAFFLE:
                if(command.getActiveEntity().has_value()){
                    if(command.getActiveEntity().value() != expectedRequestType.getEntityId() ||
                        expectedRequestType.getTurnType() != TurnType::ACTION){
                        log.warn("Received request not allowed: Wrong entity or no action allowed");
                        return false;
                    }

                    try{
                        auto player = std::dynamic_pointer_cast<gameModel::Chaser>(environment->getPlayerById(command.getActiveEntity().value()));
                        auto targetPlayer = environment->getPlayer(environment->quaffle->position);
                        if(!player || !targetPlayer.has_value()){
                            log.warn("Wresting player is no Chaser or no player on target");
                            return false;
                        }

                        gameController::WrestQuaffle wQuaffle(environment, player, environment->quaffle->position);
                        if(wQuaffle.check() == gameController::ActionCheckResult::Impossible){
                            log.warn("Wrest is impossible");
                            return false;
                        }

                        auto res = wQuaffle.execute();
                        addFouls(res.second, player);
                        if(res.first.size() > 1){
                            throw std::runtime_error(std::string{"Unexpected action result"});
                        }

                        return true;
                    } catch(std::exception &e){
                        throw std::runtime_error(e.what());
                    }
                } else {
                    log.warn("Wrest request has insufficient information");
                    return false;
                }

            case DeltaType::FOOL_AWAY:
            case DeltaType::TURN_USED:
            case DeltaType::PHASE_CHANGE:
            case DeltaType::GOAL_POINTS_CHANGE:
            case DeltaType::ROUND_CHANGE:
            case DeltaType::BAN:
            case DeltaType::BLUDGER_KNOCKOUT:
            case DeltaType::REMOVE_POO:
                log.warn("Illegal delta request type");
                return false;
            default:
                throw std::runtime_error(std::string("Fatal error, DeltaType out of range! Possible memory corruption!"));
        }
    }

    void Game::executeBallDelta(communication::messages::types::EntityId entityId){
        std::shared_ptr<gameModel::Ball> ball;
        using namespace communication::messages::types;

        if (entityId == EntityId::BLUDGER1 ||
            entityId == EntityId::BLUDGER2) {
            try{
                ball = environment->getBallByID(entityId);
                std::shared_ptr<gameModel::Bludger> bludger = std::dynamic_pointer_cast<gameModel::Bludger>(ball);

                if(!bludger){
                    throw std::runtime_error(std::string{"We done fucked it up!"});
                }

                log.debug(toString(entityId) + " moves. Position before move: [" +
                     std::to_string(bludger->position.x) + ", " + std::to_string(bludger->position.y) + "]");
                auto res = gameController::moveBludger(bludger, environment);
                log.debug(toString(entityId) + " moves. Position after move: [" +
                    std::to_string(bludger->position.x) + ", " + std::to_string(bludger->position.y) + "]");
                if(res.has_value()){
                    log.debug("Bludger knocked out a player and was redeployed");
                }
            } catch (std::exception &e){
                throw std::runtime_error(e.what());
            }

        } else if (entityId == EntityId::SNITCH) {
            ball = environment->snitch;
            auto snitch = std::dynamic_pointer_cast<gameModel::Snitch>(ball);
            if(!snitch){
                throw std::runtime_error(std::string{"We done fucked it up!"});
            }

            bool caught = gameController::moveSnitch(snitch, environment, overTimeState);
            if(caught){
                auto catcher = environment->getPlayer(environment->snitch->position);
                if(!catcher.has_value()){
                    throw std::runtime_error(std::string{"Fatal error! Snitch did not collide with a seeker"});
                }

                auto winningTeam = getVictoriousTeam(catcher.value());
                winEvent.emplace(winningTeam.first, winningTeam.second);
            }
        } else {
            throw std::runtime_error("Quaffle or !ball passed to executeBallDelta!");
        }
    }

    void Game::changePhase() {
        log.debug("Phase over");
        gameController::moveQuaffelAfterGoal(environment);

    }

    void Game::endRound() {
        using namespace communication::messages::types;
        log.debug("Round over");
        goalScored = false;
        currentPhase = PhaseType::BALL_PHASE;
        roundNumber++;
        phaseManager.reset();
        environment->removeDeprecatedShit();
        playersUsedRight.clear();
        playersUsedLeft.clear();

        if(environment->team1->numberOfBannedMembers() > MAX_BAN_COUNT &&
            environment->team2->numberOfBannedMembers() > MAX_BAN_COUNT) {
            auto winningTeam = getVictoriousTeam(environment->team1->keeper);
            if(winningTeam.second != VictoryReason::MOST_POINTS) {
                if(!firstSideDisqualified.has_value()){
                    throw std::runtime_error("Fatal error, inconsistent game state");
                }

                auto winningSide = firstSideDisqualified.value() == gameModel::TeamSide::LEFT ? gameModel::TeamSide::RIGHT : gameModel::TeamSide::LEFT;
                winEvent.emplace(winningSide, VictoryReason::BOTH_DISQUALIFICATION_POINTS_EQUAL_LAST_DISQUALIFICATION);
            } else {
                winEvent.emplace(winningTeam.first, VictoryReason::BOTH_DISQUALIFICATION_MOST_POINTS);
            }
        } else if(environment->team1->numberOfBannedMembers() > MAX_BAN_COUNT) {
            winEvent.emplace(gameModel::TeamSide::RIGHT, VictoryReason::DISQUALIFICATION);
        } else if(environment->team2->numberOfBannedMembers() > MAX_BAN_COUNT) {
            winEvent.emplace(gameModel::TeamSide::LEFT, VictoryReason::DISQUALIFICATION);
        }

        if(roundNumber == SNITCH_SPAWN_ROUND){
            gameController::spawnSnitch(environment);
        }

        switch (overTimeState){
            case gameController::ExcessLength::None:
                if(roundNumber == environment->config.getMaxRounds()){
                    overTimeState = gameController::ExcessLength::Stage1;
                }

                break;
            case gameController::ExcessLength::Stage1:
                if(++overTimeCounter > OVERTIME_INTERVAL){
                    overTimeState = gameController::ExcessLength::Stage2;
                    overTimeCounter = 0;
                }
                break;
            case gameController::ExcessLength::Stage2:
                if(environment->snitch->position == gameModel::Position{8, 6} &&
                    ++overTimeCounter > OVERTIME_INTERVAL){
                    overTimeState = gameController::ExcessLength::Stage3;
                }
                break;
            case gameController::ExcessLength::Stage3:
                break;
        }
    }

    auto Game::getVictoriousTeam(const std::shared_ptr<const gameModel::Player> &winningPlayer) const -> std::pair<gameModel::TeamSide,
    communication::messages::types::VictoryReason> {
        using namespace communication::messages::types;
        if(environment->team1->score > environment->team2->score){
            return {gameModel::TeamSide::LEFT, VictoryReason::MOST_POINTS};
        } else if(environment->team1->score < environment->team2->score){
            return {gameModel::TeamSide::RIGHT, VictoryReason::MOST_POINTS};
        } else {
            if(environment->team1->hasMember(winningPlayer)){
                return {gameModel::TeamSide::LEFT, VictoryReason::POINTS_EQUAL_SNITCH_CATCH};
            } else {
                return {gameModel::TeamSide::RIGHT, VictoryReason::POINTS_EQUAL_SNITCH_CATCH};
            }
        }
    }

    auto Game::getState() const -> aiTools::State{
        using Ftype = communication::messages::types::FanType;
        std::array<unsigned int, 5> availableFansLeft = {};
        std::array<unsigned int, 5> availableFansRight = {};
        availableFansLeft[0] = environment->getTeam(gameModel::TeamSide::LEFT)->fanblock.getUses(Ftype::ELF) - phaseManager.interferencesUsedLeft(Ftype::ELF);
        availableFansLeft[1] = environment->getTeam(gameModel::TeamSide::LEFT)->fanblock.getUses(Ftype::GOBLIN) - phaseManager.interferencesUsedLeft(Ftype::GOBLIN);
        availableFansLeft[2] = environment->getTeam(gameModel::TeamSide::LEFT)->fanblock.getUses(Ftype::TROLL) - phaseManager.interferencesUsedLeft(Ftype::TROLL);
        availableFansLeft[3] = environment->getTeam(gameModel::TeamSide::LEFT)->fanblock.getUses(Ftype::NIFFLER) - phaseManager.interferencesUsedLeft(Ftype::NIFFLER);
        availableFansLeft[4] = environment->getTeam(gameModel::TeamSide::LEFT)->fanblock.getUses(Ftype::WOMBAT) - phaseManager.interferencesUsedLeft(Ftype::WOMBAT);

        availableFansRight[0] = environment->getTeam(gameModel::TeamSide::RIGHT)->fanblock.getUses(Ftype::ELF) - phaseManager.interferencesUsedRight(Ftype::ELF);
        availableFansRight[1] = environment->getTeam(gameModel::TeamSide::RIGHT)->fanblock.getUses(Ftype::GOBLIN) - phaseManager.interferencesUsedRight(Ftype::GOBLIN);
        availableFansRight[2] = environment->getTeam(gameModel::TeamSide::RIGHT)->fanblock.getUses(Ftype::TROLL) - phaseManager.interferencesUsedRight(Ftype::TROLL);
        availableFansRight[3] = environment->getTeam(gameModel::TeamSide::RIGHT)->fanblock.getUses(Ftype::NIFFLER) - phaseManager.interferencesUsedRight(Ftype::NIFFLER);
        availableFansRight[4] = environment->getTeam(gameModel::TeamSide::RIGHT)->fanblock.getUses(Ftype::WOMBAT) - phaseManager.interferencesUsedRight(Ftype::WOMBAT);

        return {environment->clone(), roundNumber, currentPhase, overTimeState, overTimeCounter, goalScored, playersUsedLeft, playersUsedRight, availableFansLeft, availableFansRight};
    }

    auto Game::getUsedPlayers(const gameModel::TeamSide &side) ->
        std::unordered_set<communication::messages::types::EntityId> & {
        return side == gameModel::TeamSide::LEFT ? playersUsedLeft : playersUsedRight;
    }

    void Game::saveState(const aiTools::State &state, const std::string &path) const {
        nlohmann::json j;
        j = state;
        auto data = j.dump();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch());
        std::ofstream file(path + "state_" + std::to_string(ms.count()) + ".json");
        file << data;
        file.close();
    }

    void Game::saveExperience() const {
        using namespace communication::messages::types;
        auto currentState = getState();
        auto playerOnQuaffle = currentState.env->getPlayer(currentState.env->quaffle->position);
        auto playerOnBludger0 = currentState.env->getPlayer(currentState.env->bludgers[0]->position);
        auto playerOnBludger1 = currentState.env->getPlayer(currentState.env->bludgers[1]->position);
        auto snitchExists = currentState.env->snitch->exists && ballTurn == EntityId::BLUDGER1 &&
                currentState.currentPhase == PhaseType::BALL_PHASE;

        auto notUsed = [&currentState](const std::shared_ptr<gameModel::Player> &p){
            return currentState.playersUsedLeft.find(p->getId()) == currentState.playersUsedLeft.end() &&
                currentState.playersUsedRight.find(p->getId()) == currentState.playersUsedRight.end();
        };

        auto qThrowPossible = playerOnQuaffle.has_value() && !(*playerOnQuaffle)->knockedOut && notUsed(*playerOnQuaffle);
        auto bShotPossible = (playerOnBludger0.has_value() && INSTANCE_OF(*playerOnBludger0, gameModel::Beater) && notUsed(*playerOnBludger0)) ||
                (playerOnBludger1.has_value() && INSTANCE_OF(*playerOnBludger1, gameModel::Beater) && notUsed(*playerOnBludger1));
        if(qThrowPossible || bShotPossible || snitchExists){
            saveState(currentState, experienceDirectory);
            if(qThrowPossible){
                log.info("Throw possible, saving state");
            }

            if(bShotPossible){
                log.info("Bludger shot possible, saving state");
            }

            if(snitchExists){
                log.info("Snitch exists, saving state");
            }
        }
    }

}
