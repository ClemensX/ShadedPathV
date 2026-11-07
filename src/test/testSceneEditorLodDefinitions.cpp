#include "mainheader.h"
#include "../app/SceneEditorLodDefinitions.h"
#include <gtest/gtest.h>
#include <limits>

TEST(SceneEditorLodDefinitions, FormatsEditedValuesAndCategoryWithoutDistances)
{
    LodEntry entry{ 7, 2, 0.125f, 42.0f, { 0.0f, 1.5f, -1.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f } };
    const std::vector<SceneEditorLodDefinition> definitions{
        { 7, "tree \"A\"\\path\n", entry },
        { 100, "invisible", std::nullopt },
        { 0, "New category", LodEntry{} }
    };

    const std::string expected =
        "inline constexpr LodCategoryDef LOD_CATEGORIES[] = {\n"
        "    { 0, \"New category\" },\n"
        "    { 7, \"tree \\\"A\\\"\\\\path\\012\" },\n"
        "    { 100, \"invisible\" },\n"
        "};\n\n"
        "#define LOD_ENTRIES_COUNT 2\n\n"
        "LodEntry LOD_ENTRIES[LOD_ENTRIES_COUNT] = {\n"
        "    { 7, 2, 0.125f, 42.0f, { 0.0f, 1.5f, -1.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f } },\n"
        "    { 0, 0, 0.0f, 0.0f, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } },\n"
        "};\n";
    EXPECT_EQ(formatLodDefinitions(definitions), expected);
}

TEST(SceneEditorLodDefinitions, FormatsNonFiniteValues)
{
    LodEntry entry{};
    entry.minDistance = -std::numeric_limits<float>::infinity();
    entry.maxDistance = std::numeric_limits<float>::infinity();
    entry.dist_map[0] = std::numeric_limits<float>::quiet_NaN();
    const std::string text = formatLodDefinitions({ { 0, "test", entry } });
    EXPECT_NE(text.find("-std::numeric_limits<float>::infinity()"), std::string::npos);
    EXPECT_NE(text.find(", std::numeric_limits<float>::infinity()"), std::string::npos);
    EXPECT_NE(text.find("std::numeric_limits<float>::quiet_NaN()"), std::string::npos);
}
