#pragma once

// generate billboards to teview mass rendering
class TextureViewer : ShadedPathApplication, public AppSupport
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
    void buildCustomUI() override;
private:
    ShadedPathEngine engine;
    Shaders& shaders = engine.shaders;
    void updatePerFrame(ThreadResources& tr);
    std::vector<std::string> textureNames;
};
