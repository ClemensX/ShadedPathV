#pragma once

// generate billboards to teview mass rendering
class TextureViewer : public ShadedPathApplication, public AppSupport
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
    void buildCustomUI() override;
private:
    void updatePerFrame(ThreadResources& tr);
    std::vector<std::string> textureNames;
    std::string helpText =
        "click to see texture names\n(leftmost on top)";
};
