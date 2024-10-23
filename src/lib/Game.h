#pragma once

// QoL features for games

struct GamePhase {
    int phase = 0;
    std::string name = "";
};

class Game {
public:
    void addGamePhase(int p, std::string pName) {
        GamePhase gp;
        gp.phase = p;
        gp.name = pName;
        gamePhases.push_back(gp);
    }
    void setPhase(int p) {
        for (int i = 0; i < gamePhases.size(); i++) {
            if (gamePhases[i].phase == p) {
                Log("Setting game phase to " << gamePhases[i].name);
                currentPhase = &gamePhases[i];
                return;
            }
        }
        Error("Game phase not found");
    }
private:
    std::vector<GamePhase> gamePhases;
    GamePhase* currentPhase;
};
