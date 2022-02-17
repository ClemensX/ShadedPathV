#pragma once
class SimpleApp : ShadedPathApplication
{
public:
    void run();
    void drawFrame(ThreadResources& tr);
private:
    ShadedPathEngine engine;
    void updatePerFrame(ThreadResources& tr);
};

