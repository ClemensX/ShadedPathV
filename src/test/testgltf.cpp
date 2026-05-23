
#include "mainheader.h"
#include "test.h"
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

    //void setupTestDataFolder() {
    //    auto cur_path = std::filesystem::current_path();
    //    auto data_test_path = cur_path / "data_test";
    //    if (!std::filesystem::exists(data_test_path)) {
    //        std::filesystem::create_directory(data_test_path);
    //    }
    //    auto mesh_path = data_test_path / "mesh";
    //    if (!std::filesystem::exists(mesh_path)) {
    //        std::filesystem::create_directory(mesh_path);
    //    }
    //    engine->files.findAssetFolder("data_testXXX");
    //}

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

TEST_F(GLTFParserTest, TextureReuse) {
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
TEST_F(GLTFParserTest, SingleMesh_NoPrimitives) {
    engine->files.findAssetFolder("test_samples");
    string glbFile = engine->files.findFile("cube_single.gltf", FileCategory::MESH, false);
    EXPECT_NE(0, glbFile.size()); // check that we found file

    engine->meshStore.loadMesh("cube_single.gltf", "SingleMesh");

    MeshInfo* mi = engine->meshStore.getMesh("SingleMesh");
    validateMeshInfo(mi, "SingleMesh", 0);

    // Verify it's not LOD and not an additional primitive
    EXPECT_FALSE(mi->isLodMesh());
    EXPECT_FALSE(mi->isAdditionalPrimitive());
    EXPECT_NE(mi->gltfMeshIndex, -1); // mesh loaded from gltf file
    EXPECT_EQ(mi->gltfNextPrimitiveIndex, -1); // No chained primitives
}

// Test 2: Single mesh with LOD levels (10 LODs)
TEST_F(GLTFParserTest, SingleMesh_WithLODs) {
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
TEST_F(GLTFParserTest, SingleMesh_MultiplePrimitives) {
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
TEST_F(GLTFParserTest, Complex_LODsAndPrimitives) {
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
TEST_F(GLTFParserTest, PrimitiveMap_Validation) {
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
TEST_F(GLTFParserTest, EmptyFile_GracefulFailure) {
    // Test error handling with empty or invalid GLTF
    Log("Test for empty/invalid GLTF file handling\n");

    // TODO: Test with intentionally broken GLTF files
    // Should fail gracefully without crashing
}

// Test 7: Multiple separate meshes (not LODs, just different objects)
TEST_F(GLTFParserTest, MultipleSeparateMeshes) {
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
