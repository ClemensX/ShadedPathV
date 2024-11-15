#pragma once

// example for loading and rendering gltf models
class Incoming : ShadedPathApplication, public AppSupport
{
public:
    enum class RockWave { Cube };
    struct RockInfo {
        glm::vec3 pos;
        glm::vec3 scale;
        glm::vec3 rot;
        std::string subName; // e.g. 'Rocks.3'
    };

    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
    void buildCustomUI() override;
    void addRandomRockFormations(RockWave waveName, std::vector<WorldObject*>& rockList);
    void addRandomRock(RockInfo ri, std::vector<WorldObject*>& rockList);

private:
    void updatePerFrame(ThreadResources& tr);
    WorldObject* worldObject = nullptr;
    WorldObject* gun = nullptr;
    bool holdWeapon = false;
    bool processGunshot = false;
    float pickupDistance = 2.3f;
    glm::vec3 r = glm::vec3(-0.35, 1.5, -0.425);
    Game game;
    unsigned long uiVerticesTotal = 0;
    unsigned long uiVerticesSqrt = 0;
    void handleKeyInputTurnWeapon(bool shift, int mods, int key);
    void handleKeyInputIntersectTest(bool shift, int mods, int key);
    bool enableIntersectTest = false;
    bool intersectTestModifyStartPoint = false;
    LineDef intersectTestLine;
    LineDef shootLine;
    std::vector<WorldObject*> rockObjects;

    // game phases:
    static const int PhaseIntro = 0;
    static const int PhasePrepare = 1;
    static const int PhasePhase1 = 2;
    static const int PhasePhase2 = 3;
    static const int PhasePhase3 = 4;
    static const int PhaseEnd = 5;
    static const int PhaseEndTitles = 6;

    // object groups:
    static const int GroupRocks = 0;
    static const int GroupGun = 1;
    static const int GroupTerrain = 2;
    static const int GroupDebug = 3;
    const char* GroupRocksName = "GroupRocks";
    const char* GroupGunName = "GroupGun";
    const char* GroupTerrainName = "GroupTerrain";
    const char* GroupDebugName = "GroupDebug";
};

