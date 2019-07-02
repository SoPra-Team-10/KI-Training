//
// Created by bjorn on 02.05.19.
//

#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <SopraMessages/MatchConfig.hpp>
#include <SopraMessages/Next.hpp>
#include <SopraMessages/TeamConfig.hpp>
#include <SopraMessages/TeamFormation.hpp>
#include <SopraMessages/DeltaRequest.hpp>
#include <SopraMessages/Snapshot.hpp>
#include <SopraGameLogic/GameModel.h>
#include <chrono>
#include <SopraUtil/Logging.hpp>
#include "GameTypes.h"
#include <SopraUtil/Timer.h>
#include "PhaseManager.h"
#include <unordered_set>
#include <AI/AI.h>

namespace gameHandling {
    constexpr auto SNITCH_SPAWN_ROUND = 10;
    constexpr auto OVERTIME_INTERVAL = 3;
    constexpr auto MAX_BAN_COUNT = 2;

    class Game {
    public:
        std::shared_ptr<gameModel::Environment> environment;
        Game(communication::messages::broadcast::MatchConfig matchConfig,
             const communication::messages::request::TeamConfig& teamConfig1,
             const communication::messages::request::TeamConfig& teamConfig2,
             communication::messages::request::TeamFormation teamFormation1,
             communication::messages::request::TeamFormation teamFormation2,
             util::Logging &log, std::string expPath);

        mutable std::optional<std::pair<gameModel::TeamSide, communication::messages::types::VictoryReason>> winEvent;

        /**
         * Gets the next actor to make a move. If the actor is a player, the timeout timer is started
         * @return
         */
        auto getNextAction() -> communication::messages::broadcast::Next;

        /**
         * Executess the requested command if compliant with the game rules
         * @param command Command to be executed
         * @param teamSide Side which executes the command
         * @return true if successful, false if rule violation
         */
        bool executeDelta(communication::messages::request::DeltaRequest command, gameModel::TeamSide teamSide);

        /**
         * Executes a ball turn
         * @param entityId the ball to make a move
         */
        void executeBallDelta(communication::messages::types::EntityId entityId);

        /**
         * Gets a copy of the current game state
         * @return
         */
        auto getState() const -> aiTools::State;

        /**
         * Saves the current State if certain conditions are met
         */
        void saveExperience() const;

    private:
        communication::messages::types::PhaseType currentPhase = communication::messages::types::PhaseType::BALL_PHASE; ///< the basic game phases
        communication::messages::types::EntityId ballTurn =
                communication::messages::types::EntityId::SNITCH; ///< the Ball to make a move
        unsigned int roundNumber = 1;
        Timeouts timeouts;
        PhaseManager phaseManager;
        communication::messages::broadcast::Next expectedRequestType{}; ///<Next-object containing information about the next expected request from a client
        gameModel::TeamSide currentSide; ///<Current side to make a move
        util::Logging &log;
        gameController::ExcessLength overTimeState = gameController::ExcessLength::None;
        unsigned int overTimeCounter = 0;
        bool goalScored = false;
        std::deque<std::shared_ptr<gameModel::Player>> bannedPlayers = {};
        std::optional<gameModel::TeamSide> firstSideDisqualified = std::nullopt;
        std::unordered_set<communication::messages::types::EntityId> playersUsedLeft = {};
        std::unordered_set<communication::messages::types::EntityId> playersUsedRight = {};
        std::string experienceDirectory;

        auto getUsedPlayers(const gameModel::TeamSide &side) -> std::unordered_set<communication::messages::types::EntityId>&;

        /**
         * gets the winning Team and the reason for winning when the snitch has been caught.
         * @param winningPlayer the Player catching the snitch
         * @return the winning team according to the game rules and the reason they won
         */
        auto getVictoriousTeam(const std::shared_ptr<const gameModel::Player> &winningPlayer) const -> std::pair<gameModel::TeamSide, communication::messages::types::VictoryReason>;

        /**
         * pushes a PHASE_CHANGE DeltaBroadcast on the lastDeltas queue if the game phase has changed
         * and makes necessary changes to the environment for the next phase
         */
        void changePhase();

        /**
         * Prepares the game state for the next round.
         */
        void endRound();

        /**
         * Saves a game state to the specified directory
         * @param state The state to be saved
         * @param path path to directory where state is to be saved
         */
        void saveState(const aiTools::State &state, const std::string &path) const;
    };
}


#endif //SERVER_GAME_H
