
#include "mainheader.h"
#include "test.h"
#include "TextureAnalyzer.h"
//#include <gtest/gtest.h>


using namespace std;
using namespace glm;

class GLTFParserTest : public WorkingDirectoryTest {
protected:
    ShadedPathEngine* engine = nullptr;

    void SetUp() override {
        WorkingDirectoryTest::SetUp();

        // Create engine instance
        engine = new ShadedPathEngine();
        minimalEngineInitialization(engine);

        // Set up test data folder structure
        //setupTestDataFolder();
        engine->files.findAssetFolder("test_samples");
    }

    void TearDown() override {
        if (engine) {
            delete engine;
            engine = nullptr;
        }
        WorkingDirectoryTest::TearDown();
    }
};

class GLTF_OLD : public WorkingDirectoryTest {
protected:
    ShadedPathEngine* engine = nullptr;

    void SetUp() override {
        WorkingDirectoryTest::SetUp();

        // Create engine instance
        engine = new ShadedPathEngine();
        minimalEngineInitialization(engine);

        // Set up test data folder structure
        //setupTestDataFolder();
        engine->files.findAssetFolder("test_samples");
    }

    void TearDown() override {
        if (engine) {
            delete engine;
            engine = nullptr;
        }
        WorkingDirectoryTest::TearDown();
    }

    // Helper to validate basic MeshInfo structure
    void validateMeshInfo(MeshInfo* mi, const std::string& expectedId, int expectedPrimitiveIndex = 0) {
        ASSERT_NE(mi, nullptr) << "MeshInfo should not be null for id: " << expectedId;
        EXPECT_EQ(mi->id, expectedId);
        EXPECT_TRUE(mi->available) << "Mesh should be available: " << expectedId;
        EXPECT_EQ(mi->gltfPrimitiveIndex, expectedPrimitiveIndex);
        EXPECT_GT(mi->vertices.size(), 0) << "Mesh should have vertices: " << expectedId;
        EXPECT_GT(mi->indices.size(), 0) << "Mesh should have indices: " << expectedId;
    }

    // Helper to validate MeshCollection structure
    void validateCollection(MeshCollection* coll, const std::string& expectedId) {
        ASSERT_NE(coll, nullptr) << "MeshCollection should not be null for id: " << expectedId;
        EXPECT_EQ(coll->id, expectedId);
        EXPECT_TRUE(coll->available);
        EXPECT_GT(coll->meshCount(), 0) << "Collection should contain meshes";
    }

    // Helper to count major meshes (non-additional primitives)
    int countMajorMeshes(MeshCollection* coll) {
        int count = 0;
        for (auto* mi : *coll) {
            if (!mi->isAdditionalPrimitive()) {
                count++;
            }
        }
        return count;
    }

    // Helper to validate LOD structure
    void validateLODStructure(MeshCollection* coll, int expectedLODCount) {
        int majorCount = countMajorMeshes(coll);
        EXPECT_EQ(majorCount, expectedLODCount)
            << "Expected " << expectedLODCount << " LOD levels, found " << majorCount;

        // Verify LOD flags are set
        for (auto* mi : *coll) {
            if (majorCount == 10) {
                EXPECT_TRUE(mi->isLodMesh()) << "Mesh should have LOD flag: " << mi->id;
            }
        }

        // Verify primitive map is filled
        coll->logLodMeshes();
    }

    // Helper to validate primitive chaining
    void validatePrimitiveChain(MeshInfo* firstPrimitive, int expectedChainLength) {
        ASSERT_NE(firstPrimitive, nullptr);
        EXPECT_EQ(firstPrimitive->gltfPrimitiveIndex, 0)
            << "First mesh in chain should have primitive index 0";

        int chainLength = 1;
        MeshInfo* current = firstPrimitive;

        while (current->gltfNextPrimitiveIndex >= 0) {
            chainLength++;
            int nextIndex = current->gltfNextPrimitiveIndex;
            auto* coll = engine->meshStore.meshCollectionStore.getMeshCollectionByIndex(
                current->collectionStoreIndex);
            ASSERT_NE(coll, nullptr);
            current = coll->getMeshInfoAt(nextIndex);
            ASSERT_NE(current, nullptr) << "Chained primitive should exist";
        }

        EXPECT_EQ(chainLength, expectedChainLength)
            << "Expected primitive chain length: " << expectedChainLength
            << ", found: " << chainLength;
    }
};

TEST_F(GLTF_OLD, TextureReuse) {
    string glbFile = engine->files.findFile("cube_single.gltf", FileCategory::MESH, false);
    EXPECT_NE(0, glbFile.size()); // check that we found file

    int cur_global_textures = engine->textureStore.size();
    engine->meshStore.loadMesh("cube_single.gltf", "SingleMesh");
    int textures_after_mesh_loading = engine->textureStore.size();
    EXPECT_EQ(textures_after_mesh_loading, cur_global_textures + 3) << "Expected 3 new texture to be loaded";

    // now load the same mesh again with a different name - should reuse textures:
    //engine->meshStore.loadMesh("mesh_with_lods.gltf", "SingleMeshCopy");
    engine->meshStore.loadMesh("cube_single.gltf", "SingleMeshCopy");
    int textures_after_second_load = engine->textureStore.size();
    EXPECT_EQ(textures_after_second_load, textures_after_mesh_loading) << "Expected textures to be reused";

}


// Test 1: Single mesh with no primitives or LODs
TEST_F(GLTF_OLD, SingleMesh_NoPrimitives) {
    engine->files.findAssetFolder("test_samples");
    string glbFile = engine->files.findFile("cube_single.gltf", FileCategory::MESH, false);
    EXPECT_NE(0, glbFile.size()); // check that we found file

    engine->meshStore.loadMesh("cube_single.gltf", "SingleMesh");

    MeshInfo* mi = engine->meshStore.getMesh("SingleMesh");
    ASSERT_NE(mi, nullptr) << "MeshInfo should not be null for id: SingleMesh";
    validateMeshInfo(mi, "SingleMesh", 0);

    // Verify it's not LOD and not an additional primitive
    EXPECT_FALSE(mi->isLodMesh());
    EXPECT_FALSE(mi->isAdditionalPrimitive());
    EXPECT_NE(mi->gltfMeshIndex, -1); // mesh loaded from gltf file
    EXPECT_EQ(mi->gltfNextPrimitiveIndex, -1); // No chained primitives
}

// Test new gltf implementation
TEST_F(GLTFParserTest, SingleMesh_NoPrimitives) {
    MStore& mstore = engine->mstore;
    engine->files.findAssetFolder("test_samples");
    string glbFile = engine->files.findFile("cube_single.gltf", FileCategory::MESH, false);
    EXPECT_NE(0, glbFile.size()); // check that we found file

    int cur_global_textures = engine->textureStore.size();
    auto meshCount = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos);
    engine->mstore.loadMesh("cube_single.gltf", "SingleMesh");
    int textures_after_mesh_loading = engine->textureStore.size();

    EXPECT_EQ(textures_after_mesh_loading, cur_global_textures + 3) << "Expected 3 new texture to be loaded";

    // check mesh count in file:
    MeshFile* meshFile = mstore.getMeshFileByID("SingleMesh");
    EXPECT_EQ(meshFile->meshes.size(), 1) << "Expected 1 mesh in gltf file cube_single.gltf";

    // access textures of the mesh:
    size_t textureCount = mstore.gltf.getTextureCount();
    EXPECT_EQ(textureCount, 3) << "Expected 3 textures for SingleMesh";
    for (size_t i = 0; i < textureCount; ++i) {
        auto globIdx = mstore.gltf.getGlobalTextureIndex(static_cast<int>(i));
        auto* texInfo = engine->textureStore.getTextureByIndex(static_cast<uint32_t>(globIdx));
        Log("Texture " << i << ": global index = " << globIdx << ", id = " << texInfo->id << ", filename = " << texInfo->filename << "\n");
    }
    auto meshCountAfterLoad = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos);
    EXPECT_GT(meshCountAfterLoad, meshCount) << "Expected mesh count to increase after loading";

    auto loaded = mstore.getMeshFileByID("SingleMesh"); // ensure we can retrieve the mesh file by ID
    EXPECT_EQ(loaded->meshes.size(), 1); // should be 1 mesh
    auto meshInfo = mstore.getGPUMeshInfo(loaded->meshes[0].meshIndex);
    const auto meshMetadata = mstore.getMeshMetadata(loaded->meshes[0].meshIndex);
    EXPECT_EQ(meshMetadata->name, "Cube");

    // access newest mesh info, from the file just loaded:
    const GPUMaterial* material = mstore.getGPUMaterial(meshInfo->material);
    EXPECT_GT(material->baseColorTextureSet, 0);
    EXPECT_GT(material->physicalDescriptorTextureSet, 0);
    EXPECT_GT(material->normalTextureSet, 0);
    EXPECT_TRUE(material->occlusionTextureSet == -1);
    EXPECT_TRUE(material->emissiveTextureSet == -1);

    auto* texInfo = engine->textureStore.getTextureByIndex(static_cast<uint32_t>(material->baseColorTextureSet));
    //Log("Texture: global index = " << material->baseColorTextureSet << ", id = " << texInfo->id << ", filename = " << texInfo->filename << "\n");

    // Example validations:
    // 1. Check if texture has expected color distribution
    // glm::vec4 expectedMean(0.5f, 0.5f, 0.5f, 1.0f); // Gray with full alpha
    // auto colorResult = TextureAnalyzer::validateColorRange(stats, expectedMean, 0.2f);
    // EXPECT_TRUE(colorResult.passed) << colorResult.message;

    // 2. Check if texture has reasonable variance (not solid color or too noisy)
    // auto varianceResult = TextureAnalyzer::validateVariance(stats, 0.05f, 0.3f);
    // EXPECT_TRUE(varianceResult.passed) << varianceResult.message;

    // 3. Check if texture is mostly a solid color
    // bool isSolid = TextureAnalyzer::isSolidColor(stats);
    // Log("Is solid color: " << (isSolid ? "yes" : "no") << "\n");

    // Analyze texture data using TextureAnalyzer
    
    auto lightRedColor = glm::vec3(0.80f, 0.32f, 0.32f);
    auto darkRedColor = glm::vec3(0.40f, 0.149f, 0.149f);
    auto greenColor = glm::vec3(0.0f, 0.502f, 0.0f);
    auto normalColor = glm::vec3(0.502f, 0.502f, 1.0f);
    auto stats = TextureAnalyzer::analyzeTexture(engine, texInfo);
    //TextureAnalyzer::printStats(stats, "BaseColor Texture");

    auto colorResult = TextureAnalyzer::validateDominantColor(stats, lightRedColor, 0.50f);
    EXPECT_TRUE(colorResult.passed) << colorResult.message;
    colorResult = TextureAnalyzer::validateDominantColor(stats, darkRedColor, 0.50f);
    EXPECT_TRUE(colorResult.passed) << colorResult.message;

    texInfo = engine->textureStore.getTextureByIndex(static_cast<uint32_t>(material->physicalDescriptorTextureSet));
    stats = TextureAnalyzer::analyzeTexture(engine, texInfo);
    EXPECT_TRUE(TextureAnalyzer::isSolidColor(stats));
    colorResult = TextureAnalyzer::validateDominantColor(stats, greenColor, 1.00f);
    EXPECT_TRUE(colorResult.passed) << colorResult.message;

    texInfo = engine->textureStore.getTextureByIndex(static_cast<uint32_t>(material->normalTextureSet));
    stats = TextureAnalyzer::analyzeTexture(engine, texInfo);
    EXPECT_TRUE(TextureAnalyzer::isSolidColor(stats));
    colorResult = TextureAnalyzer::validateDominantColor(stats, normalColor, 1.00f);
    EXPECT_TRUE(colorResult.passed) << colorResult.message;

    // check vertices and indices
    EXPECT_GT(meshMetadata->vertices.size(), 0);
    EXPECT_GT(meshMetadata->indices.size(), 0);
}

// Test access to mesh info, textures, model and material
TEST_F(GLTFParserTest, SingleMesh_CheckShaderData) {
    MStore& mstore = engine->mstore;
    engine->mstore.loadMesh("cube_single.gltf", "SingleMesh");

    MeshFile* meshFile = mstore.getMeshFileByID("SingleMesh");
    int32_t meshIndex = meshFile->meshes[0].meshIndex;
    GPUMeshInfo* meshInfo = mstore.getGPUMeshInfo(meshIndex);
    MeshInfoMetadata* meshMetadata = mstore.getMeshMetadata(meshIndex);
    GPUMaterial* material = mstore.getGPUMaterial(meshInfo->material);

    EXPECT_NE(meshInfo, nullptr);
    EXPECT_NE(meshMetadata, nullptr);
    EXPECT_NE(material, nullptr);

    // material and textures
    EXPECT_GT(material->baseColorTextureSet, 0);
    EXPECT_GT(material->physicalDescriptorTextureSet, 0);
    EXPECT_GT(material->normalTextureSet, 0);
    TextureInfo* texInfo = engine->textureStore.getTextureByIndex(static_cast<uint32_t>(material->baseColorTextureSet));
    EXPECT_NE(texInfo, nullptr);
    texInfo = engine->textureStore.getTextureByIndex(static_cast<uint32_t>(material->physicalDescriptorTextureSet));
    EXPECT_NE(texInfo, nullptr);
    texInfo = engine->textureStore.getTextureByIndex(static_cast<uint32_t>(material->normalTextureSet));
    EXPECT_NE(texInfo, nullptr);

    // mesh vertex data:
    EXPECT_EQ(meshMetadata->vertices.size(), 24); // cube_single.gltf has 24 vertices
    EXPECT_EQ(meshMetadata->indices.size(), 36); // cube_single.gltf has 36 indices (12 triangles)
    mstore.getBoundingBox(meshInfo->boundingBox, *meshInfo);
    EXPECT_EQ(meshInfo->boundingBox.min, glm::vec3(-1.0f, -1.0f, -1.0f));
    EXPECT_EQ(meshInfo->boundingBox.max, glm::vec3(1.0f, 1.0f, 1.0f));

    // stationary objects:
    for (int i = 0; i < engine->getMaxObjects(); ++i) {
        auto obj = engine->mstore.addObject(meshIndex, glm::vec3(0.0f, 0.0f, 0.0f));
        EXPECT_TRUE(obj != nullptr) << "Failed to add object at index " << i;
        EXPECT_EQ(obj->index, i);
    }
    // adding another object should exit()
    //engine->mstore.addObject(meshIndex, glm::vec3(0.0f, 0.0f, 0.0f));

    // moving objects:
    for (int i = 0; i < engine->getMaxMovingObjects(); ++i) {
        MeshFlagsCollection flagsMoving;
        flagsMoving.setFlag(MeshFlags::RENDER_TYPE_MOVING);
        auto obj = engine->mstore.addObject(meshIndex, glm::vec3(0.0f, 0.0f, 0.0f), flagsMoving);
        EXPECT_TRUE(obj != nullptr) << "Failed to add moving object at index " << i;
        EXPECT_EQ(obj->index, i);
    }
}

// Test access to mesh info, textures, model and material
TEST_F(GLTFParserTest, Meshlets) {
    MStore& mstore = engine->mstore;
    engine->mstore.loadMesh("cube_single.gltf", "SingleMesh");
    MeshFile* meshFile = mstore.getMeshFileByID("SingleMesh");
    int32_t meshIndex = meshFile->meshes[0].meshIndex;
    MeshInfoMetadata* meshMetadata = mstore.getMeshMetadata(meshIndex);

    EXPECT_FALSE(meshMetadata->hasMeshlets()) << "cube_single.gltf should not have meshlets without generating them";

    // now load again with meshlet generation enabled:
    MeshFlagsCollection flags;
    flags.setFlag(MeshFlags::MESHLET_GENERATE);
    engine->mstore.loadMesh("cube_single.gltf", "SingleMesh_Meshlets", flags);
    meshFile = mstore.getMeshFileByID("SingleMesh_Meshlets");
    meshIndex = meshFile->meshes[0].meshIndex;
    meshMetadata = mstore.getMeshMetadata(meshIndex);

    EXPECT_TRUE(meshMetadata->hasMeshlets()) << "cube_single.gltf meshlet regeneration failed";

    // upload all to GPU
    engine->shaders.pbrShader.initialUpload(true);
    auto m = mstore.getGPUMeshInfo(meshIndex);
    EXPECT_NE(m->vertexOffset, 0) << "GPUMeshInfo should not be null after upload";
}

// Test access to mesh info, textures, model and material
TEST_F(GLTFParserTest, Mesh_Indices) {
    MStore& mstore = engine->mstore;
    engine->mstore.loadMesh("cube_single.gltf", "SingleMesh");
    MeshFile* meshFile = mstore.getMeshFileByID("SingleMesh");
    int32_t meshIndex = meshFile->meshes[0].meshIndex;
    EXPECT_EQ(meshIndex, 0) << "Expected mesh index 0 for first mesh";
    MeshInfoMetadata* meshMetadata = mstore.getMeshMetadata(meshIndex);
    EXPECT_GT(meshMetadata->vertices.size(), 0) << "Expected non-zero vertex count";
    EXPECT_GT(meshMetadata->indices.size(), 0) << "Expected non-zero index count";
    GPUMeshInfo* meshInfo = mstore.getGPUMeshInfo(meshIndex);
    EXPECT_EQ(meshInfo->index, meshIndex) << "GPUMeshInfo index should match mesh index";

    // now load again and check higher indices:
    engine->mstore.loadMesh("cube_single.gltf", "SingleMesh_Meshlets");
    meshFile = mstore.getMeshFileByID("SingleMesh_Meshlets");
    meshIndex = meshFile->meshes[0].meshIndex;
    EXPECT_EQ(meshIndex, 1) << "Expected mesh index 1 for second mesh";
    MeshInfoMetadata* meshMetadata2 = mstore.getMeshMetadata(meshIndex);
    EXPECT_NE(meshMetadata2, meshMetadata) << "Expected new MeshInfoMetadata for second mesh";
    meshInfo = mstore.getGPUMeshInfo(meshIndex);
    EXPECT_EQ(meshInfo->index, meshIndex) << "GPUMeshInfo index should match mesh index for second mesh";

    // recheck GPUMeshInfo array:
    EXPECT_EQ(engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos), 2) << "Expected 2 GPUMeshInfo entries after loading two meshes";
    auto mesh0 = mstore.getGPUMeshInfo(0);
    EXPECT_EQ(mesh0->index, 0) << "First GPUMeshInfo index should be 0";
    auto mesh1 = mstore.getGPUMeshInfo(1);
    EXPECT_EQ(mesh1->index, 1) << "Second GPUMeshInfo index should be 1";
    EXPECT_NE(mesh0, mesh1) << "GPUMeshInfo entries should be distinct";
}

// Test 2: Single mesh with LOD levels (10 LODs)
TEST_F(GLTF_OLD, SingleMesh_WithLODs) {
    // This test requires a GLTF file with 10 LOD levels
    // Expected file: "mesh_with_lods.gltf" in data_test/mesh/
    // TODO: Create test file with 10 LOD meshes named mesh_lod_0 through mesh_lod_9

    // Placeholder for now - will need actual GLTF file
    // engine->meshStore.loadMeshLod("mesh_with_lods.gltf", "TestLOD");

    Log("Test requires GLTF file: mesh_with_lods.gltf with 10 LOD levels\n");
    Log("File should contain meshes: mesh_lod_0, mesh_lod_1, ... mesh_lod_9\n");

    // Once file exists, uncomment:
    /*
    auto* coll = engine->meshStore.meshCollectionStore.getMeshCollectionByIndex(0);
    validateCollection(coll, "TestLOD");
    validateLODStructure(coll, 10);

    // Verify each LOD mesh
    for (int i = 0; i < 10; i++) {
        MeshInfo* lod = coll->getMeshInfo(i, 0);
        ASSERT_NE(lod, nullptr) << "LOD level " << i << " should exist";
        EXPECT_TRUE(lod->isLodMesh());
        EXPECT_EQ(lod->gltfMeshIndex, i);
    }
    */
}

// Test 3: Single mesh with multiple primitives (e.g., tree with trunk + foliage)
TEST_F(GLTF_OLD, SingleMesh_MultiplePrimitives) {
    // This test requires a GLTF file with one mesh containing multiple primitives
    // Expected file: "tree_primitives.gltf" with 1 mesh having 2 primitives
    // Primitive 0: trunk, Primitive 1: foliage

    Log("Test requires GLTF file: tree_primitives.gltf\n");
    Log("File should contain 1 mesh with 2 primitives (trunk and foliage)\n");

    // Once file exists, uncomment:
    /*
    engine->meshStore.loadMesh("tree_primitives.gltf", "TreePrimitives");

    auto* coll = engine->meshStore.meshCollectionStore.getMeshCollectionByIndex(0);
    validateCollection(coll, "TreePrimitives");

    // Should have 2 MeshInfo objects (one per primitive)
    EXPECT_EQ(coll->meshCount(), 2);

    // Get first primitive (trunk)
    MeshInfo* trunk = engine->meshStore.getMesh("TreePrimitives");
    validateMeshInfo(trunk, "TreePrimitives", 0);
    EXPECT_FALSE(trunk->isAdditionalPrimitive());

    // Get second primitive (foliage)
    MeshInfo* foliage = coll->getMeshInfoAt(1);
    validateMeshInfo(foliage, "TreePrimitives.0#1", 1);
    EXPECT_TRUE(foliage->isAdditionalPrimitive());

    // Verify primitive chaining
    validatePrimitiveChain(trunk, 2);
    */
}

// Test 4: Multiple meshes with LODs and multiple primitives (complex case)
TEST_F(GLTF_OLD, Complex_LODsAndPrimitives) {
    // This test requires a complex GLTF file:
    // - 10 LOD levels (mesh_lod_0 through mesh_lod_9)
    // - Each LOD mesh has 2 primitives (trunk + foliage)
    // Expected file: "tree_lod_primitives.gltf"

    Log("Test requires GLTF file: tree_lod_primitives.gltf\n");
    Log("File should contain 10 meshes (LODs), each with 2 primitives\n");

    // Once file exists, uncomment:
    /*
    engine->meshStore.loadMeshLod("tree_lod_primitives.gltf", "ComplexTree");

    auto* coll = engine->meshStore.meshCollectionStore.getMeshCollectionByIndex(0);
    validateCollection(coll, "ComplexTree");
    validateLODStructure(coll, 10);

    // Should have 20 MeshInfo objects (10 LODs × 2 primitives)
    EXPECT_EQ(coll->meshCount(), 20);

    // Verify structure for each LOD
    for (int lod = 0; lod < 10; lod++) {
        // Get trunk (first primitive)
        MeshInfo* trunk = coll->getMeshInfo(lod, 0);
        ASSERT_NE(trunk, nullptr) << "Trunk for LOD " << lod << " should exist";
        EXPECT_TRUE(trunk->isLodMesh());
        EXPECT_EQ(trunk->gltfMeshIndex, lod);
        EXPECT_FALSE(trunk->isAdditionalPrimitive());

        // Get foliage (second primitive)
        MeshInfo* foliage = coll->getMeshInfo(lod, 1);
        ASSERT_NE(foliage, nullptr) << "Foliage for LOD " << lod << " should exist";
        EXPECT_TRUE(foliage->isLodMesh());
        EXPECT_TRUE(foliage->isAdditionalPrimitive());

        // Verify primitive chaining for this LOD
        validatePrimitiveChain(trunk, 2);
    }
    */
}

// Test 5: Verify primitive map correctness
TEST_F(GLTF_OLD, PrimitiveMap_Validation) {
    // This validates the LodPrimitiveMap structure after parsing
    // Using generated mesh for now

    engine->meshStore.loadMeshCylinder("MapTest",
        MeshFlagsCollection(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER));

    auto* coll = engine->meshStore.meshCollectionStore.getMeshCollectionByIndex(0);
    ASSERT_NE(coll, nullptr);

    // For single mesh, map should show 1 major mesh, 0 primitives
    EXPECT_EQ(coll->primMap.getMajorMeshCount(), 0); // Not yet filled

    coll->fillPrimitiveMap();
    // After filling, check structure
    coll->logLodMeshes();
}

// Test 6: Boundary case - Empty GLTF file
TEST_F(GLTF_OLD, EmptyFile_GracefulFailure) {
    // Test error handling with empty or invalid GLTF
    Log("Test for empty/invalid GLTF file handling\n");

    // TODO: Test with intentionally broken GLTF files
    // Should fail gracefully without crashing
}

// Test 7: Multiple separate meshes (not LODs, just different objects)
TEST_F(GLTF_OLD, MultipleSeparateMeshes) {
    // This test requires a GLTF file with multiple independent meshes
    // Expected file: "multiple_objects.gltf" with 3 different meshes
    // e.g., cube, sphere, cylinder in one file

    Log("Test requires GLTF file: multiple_objects.gltf\n");
    Log("File should contain 3 separate meshes (not LODs or primitives)\n");

    // Once file exists, uncomment:
    /*
    engine->meshStore.loadMesh("multiple_objects.gltf", "MultipleObjects");

    auto* coll = engine->meshStore.meshCollectionStore.getMeshCollectionByIndex(0);
    validateCollection(coll, "MultipleObjects");

    // Should have 3 MeshInfo objects
    EXPECT_EQ(coll->meshCount(), 3);

    // Verify each mesh is independent (no LOD, no primitive chaining)
    for (int i = 0; i < 3; i++) {
        MeshInfo* mesh = coll->getMeshInfoAt(i);
        ASSERT_NE(mesh, nullptr);
        EXPECT_EQ(mesh->gltfMeshIndex, i);
        EXPECT_FALSE(mesh->isLodMesh());
        EXPECT_EQ(mesh->gltfNextPrimitiveIndex, -1);
    }
    */
}
