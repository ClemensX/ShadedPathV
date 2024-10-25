#pragma once

// example for loading and rendering gltf models
class Incoming : ShadedPathApplication, public AppSupport
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
    void buildCustomUI() override;
    void addRandomHeightLines(std::vector<LineDef>& lines, World& world);

private:
    ShadedPathEngine engine;
    Shaders& shaders = engine.shaders;
    void updatePerFrame(ThreadResources& tr);
    WorldObject* worldObject = nullptr;
    WorldObject* gun = nullptr;
    bool holdWeapon = false;
    float pickupDistance = 2.3f;
    glm::vec3 r = glm::vec3(-0.35, 1.5, -0.425);
    Game game;
    unsigned long uiVerticesTotal = 0;
    unsigned long uiVerticesSqrt = 0;

    // game phases:
    static const int PhaseIntro = 0;
    static const int PhasePrepare = 1;
    static const int PhasePhase1 = 2;
    static const int PhasePhase2 = 3;
    static const int PhasePhase3 = 4;
    static const int PhaseEnd = 5;
    static const int PhaseEndTitles = 6;
};

