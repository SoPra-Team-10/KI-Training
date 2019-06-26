//
// Created by jonas on 07.05.19.
//

#include "ConfigCheck.h"
#include <SopraGameLogic/GameModel.h>
#include <SopraMessages/types.hpp>

namespace configCheck {
    bool checkTeamFormation(const communication::messages::request::TeamFormation &formation,
                                         const gameModel::TeamSide &side) {
        gameModel::Team team = {{"", "", "", "", "", 1,4,1,1,0,{},{},{},{},{},{},{}}, formation, side};

        auto allPlayers = team.getAllPlayers();
        if(side == gameModel::TeamSide::LEFT) {
            for (auto &player : allPlayers) {
                if (gameModel::Environment::getCell(player->position) == gameModel::Cell::OutOfBounds ||
                    player->position.x > 7 || gameModel::Environment::getCell(player->position) == gameModel::Cell::Centre) {
                    return false;
                }
            }
        }else{
            for (auto &player : allPlayers) {
                if (gameModel::Environment::getCell(player->position) == gameModel::Cell::OutOfBounds ||
                    player->position.x < 9 || gameModel::Environment::getCell(player->position) == gameModel::Cell::Centre) {
                    return false;
                }
            }
        }
        for (int i = 0; i < 7; i++){
            for (int j = i+1; j < 7; j++){
                if(allPlayers[i]->position == allPlayers[j]->position){
                    return false;
                }
            }
        }
        return true;
    }

    bool checkTeamConfig(const communication::messages::request::TeamConfig &config) {
        gameModel::Team team(config, {}, gameModel::TeamSide::LEFT);

        int tinderblast = 0;
        int cleansweep11 = 0;
        int comet260 = 0;
        int nimbus2001 = 0;
        int firebolt = 0;

        auto players = team.getAllPlayers();
        for (const auto &player : players) {
            if (player->broom == communication::messages::types::Broom::TINDERBLAST) {
                tinderblast++;
            }
            else if (player->broom == communication::messages::types::Broom::CLEANSWEEP11) {
                cleansweep11++;
            }
            else if (player->broom == communication::messages::types::Broom::COMET260) {
                comet260++;
            }
            else if (player->broom == communication::messages::types::Broom::NIMBUS2001) {
                nimbus2001++;
            }
            else if (player->broom == communication::messages::types::Broom::FIREBOLT) {
                firebolt++;
            }
            else {
                return false;
            }
        }

        // check fanblock
        if (team.fanblock.getUses(gameModel::InterferenceType::Impulse) +
            team.fanblock.getUses(gameModel::InterferenceType::SnitchPush) +
            team.fanblock.getUses(gameModel::InterferenceType::RangedAttack) +
            team.fanblock.getUses(gameModel::InterferenceType::Teleport) +
            team.fanblock.getUses(gameModel::InterferenceType::BlockCell) != 7) {
            return false;
        }
        if (team.fanblock.getUses(gameModel::InterferenceType::Impulse) < 1 ||
            team.fanblock.getUses(gameModel::InterferenceType::SnitchPush) < 1 ||
            team.fanblock.getUses(gameModel::InterferenceType::RangedAttack) < 1 ||
            team.fanblock.getUses(gameModel::InterferenceType::Teleport) < 1 ||
            team.fanblock.getUses(gameModel::InterferenceType::BlockCell) < 1) {
            return false;
        }

        // broom usage check
        return !(tinderblast < 1 || cleansweep11 < 1 || comet260 < 1 || nimbus2001 < 1 || firebolt < 1);

    }
}
