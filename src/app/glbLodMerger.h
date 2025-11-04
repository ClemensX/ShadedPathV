#pragma once

// merge glb lod files into single glb with multiple LOD meshes
class glbLodMerger : public ShadedPathApplication
{
public:
    glbLodMerger() { isCLITool = true; }
    void run(ContinuationInfo* cont) override;
    // called from main thread
    void init();
private:
};