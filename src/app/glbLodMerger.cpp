#include "mainheader.h"
#include "AppSupport.h"
#include "glbLodMerger.h"
#include "tinygltf/tiny_gltf.h"

#undef Log
#if defined(_WIN64)
#define Log(x)\
{\
    std::stringstream s1765; s1765 << x; \
    LogFile(s1765.str().c_str()); \
	printf("%s", s1765.str().c_str()); \
    std::string str = s1765.str(); \
    std::wstring wstr(str.begin(), str.end()); \
    std::wstringstream wss(wstr); \
    OutputDebugString(wss.str().c_str()); \
}
#else
#define Log(x)\
{\
    std::stringstream s1765; s1765 << x; \
    LogFile(s1765.str().c_str()); \
	printf("%s", s1765.str().c_str()); \
}
#endif
using namespace std;
using namespace glm;

void glbLodMerger::run(ContinuationInfo* cont)
{
    int argc = cont->argc;
    char** argv = cont->argv;
    if (argc < 3) {
        Log("Usage: GlbLodMergerApp <base_glb> <lod_dir>\n");
        return;
    }

    std::string baseFile = argv[1];
    std::string lodDir = argv[2];

    tinygltf::Model baseModel;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    // Load base GLB
    if (!loader.LoadBinaryFromFile(&baseModel, &err, &warn, baseFile)) {
        Log("Failed to load base GLB: " << err << "\n");
        return;
    }

    // Extract base name without extension
    std::string baseName = std::filesystem::path(baseFile).stem().string();
    //size_t lodPos = baseName.rfind("_01");
    //if (lodPos != std::string::npos) {
    //    baseName = baseName.substr(0, lodPos);
    //}
    // Find and merge LOD files
    for (int i = 2; i <= 9; ++i) {
        char lodFile[512];
        snprintf(lodFile, sizeof(lodFile), "%s/%s_%02d.glb", lodDir.c_str(), baseName.c_str(), i);
        if (!std::filesystem::exists(lodFile)) continue;

        tinygltf::Model lodModel;
        if (!loader.LoadBinaryFromFile(&lodModel, &err, &warn, lodFile)) {
            Log("Failed to load LOD GLB: " << lodFile << "\n");
            continue;
        }

        // Merge meshes from LOD model into base model
        for (auto& mesh : lodModel.meshes) {
            baseModel.meshes.push_back(mesh);
        }
        // Optionally, update nodes/scenes if needed
    }

    // Write merged GLB
    std::string outFile = baseFile.substr(0, baseFile.find_last_of('.')) + "_merged.glb";
    if (!loader.WriteGltfSceneToFile(&baseModel, outFile, true, true, true, true)) {
        Log("Failed to write merged GLB\n");
        return;
    }

    Log("Merged GLB written to: " << outFile << "\n");
    return;
}