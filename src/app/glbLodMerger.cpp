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

    // Keep track of indices of newly added meshes
    std::vector<int> newMeshIndices;

    // Find and merge LOD files
    for (int i = 1; i <= 1; ++i) {
        char lodFile[512];
        snprintf(lodFile, sizeof(lodFile), "%s/%s_%02d.glb", lodDir.c_str(), baseName.c_str(), i);
        if (!std::filesystem::exists(lodFile)) continue;

        tinygltf::Model lodModel;
        Log("Processing LOD GLB: " << lodFile << "\n");
        if (!loader.LoadBinaryFromFile(&lodModel, &err, &warn, lodFile)) {
            Log("Failed to load LOD GLB: " << lodFile << "\n");
            continue;
        }

        // Merge meshes from LOD model into base model
        for (auto& mesh : lodModel.meshes) {
            tinygltf::Mesh newMesh = mesh;
            for (auto& prim : newMesh.primitives) {
                // Copy indices accessor
                if (prim.indices >= 0) {
                    // Copy accessor
                    tinygltf::Accessor accessor = lodModel.accessors[prim.indices];
                    // Copy bufferView
                    int oldBufferViewIdx = accessor.bufferView;
                    tinygltf::BufferView bufferView = lodModel.bufferViews[oldBufferViewIdx];
                    // Copy buffer
                    int oldBufferIdx = bufferView.buffer;
                    tinygltf::Buffer buffer = lodModel.buffers[oldBufferIdx];

                    // Append buffer data
                    int newBufferIdx = int(baseModel.buffers.size());
                    baseModel.buffers.push_back(buffer);

                    // Update bufferView to point to new buffer
                    bufferView.buffer = newBufferIdx;
                    int newBufferViewIdx = int(baseModel.bufferViews.size());
                    baseModel.bufferViews.push_back(bufferView);

                    // Update accessor to point to new bufferView
                    accessor.bufferView = newBufferViewIdx;
                    int newAccessorIdx = int(baseModel.accessors.size());
                    baseModel.accessors.push_back(accessor);

                    // Update primitive to point to new accessor
                    prim.indices = newAccessorIdx;
                }

                // NOTE: If your LOD files have different vertex data, you must also copy attributes.
                // The following was intentionally left out in your current code. Uncomment and use if needed.
                // for (auto& attr : prim.attributes) {
                //     int attrAccessorIdx = attr.second;
                //     tinygltf::Accessor accessor = lodModel.accessors[attrAccessorIdx];
                //     int oldBufferViewIdx = accessor.bufferView;
                //     tinygltf::BufferView bufferView = lodModel.bufferViews[oldBufferViewIdx];
                //     int oldBufferIdx = bufferView.buffer;
                //     tinygltf::Buffer buffer = lodModel.buffers[oldBufferIdx];
                //     int newBufferIdx = int(baseModel.buffers.size());
                //     baseModel.buffers.push_back(buffer);
                //     bufferView.buffer = newBufferIdx;
                //     int newBufferViewIdx = int(baseModel.bufferViews.size());
                //     baseModel.bufferViews.push_back(bufferView);
                //     accessor.bufferView = newBufferViewIdx;
                //     int newAccessorIdx = int(baseModel.accessors.size());
                //     baseModel.accessors.push_back(accessor);
                //     attr.second = newAccessorIdx;
                // }
            }
            baseModel.meshes.push_back(newMesh);
            newMeshIndices.push_back(int(baseModel.meshes.size() - 1));
        }
    }

    // Create a parent node for all newly added meshes and attach it to the scene
    if (!newMeshIndices.empty()) {
        // Determine target scene
        int sceneIndex = baseModel.defaultScene >= 0
            ? baseModel.defaultScene
            : (!baseModel.scenes.empty() ? 0 : -1);

        if (sceneIndex == -1) {
            tinygltf::Scene sc;
            sc.name = "Scene";
            sceneIndex = int(baseModel.scenes.size());
            baseModel.scenes.push_back(sc);
            baseModel.defaultScene = sceneIndex;
        }

        // Create LOD root node
        tinygltf::Node lodRoot;
        lodRoot.name = baseName + "_LODs";
        int lodRootIndex = int(baseModel.nodes.size());
        baseModel.nodes.push_back(lodRoot);
        baseModel.scenes[sceneIndex].nodes.push_back(lodRootIndex);

        // Create a child node per new mesh
        for (size_t k = 0; k < newMeshIndices.size(); ++k) {
            tinygltf::Node child;
            child.mesh = newMeshIndices[k];
            child.name = baseName + "_LOD_" + std::to_string(k);
            int childIndex = int(baseModel.nodes.size());
            baseModel.nodes.push_back(child);
            baseModel.nodes[lodRootIndex].children.push_back(childIndex);
        }
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