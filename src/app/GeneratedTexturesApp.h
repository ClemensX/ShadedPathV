#pragma once

// view generated texture projected onto cube that can be viewed from any side
// BRDFLUT (BRDF lookup table)
// Irradiance cube map
// Environment Cube map
class GeneratedTexturesApp : ShadedPathApplication, public AppSupport
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
private:
    void updatePerFrame(ThreadResources& tr);
    //Camera* camera;
    //CameraPositioner_FirstPerson* positioner;
    //InputState input;
    World world;
    WorldObject *knife1, *knife2;
    float plus = 0.0f;
};
