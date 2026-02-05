#include "mainheader.h"
#include "WorldCreator.h"

using namespace glm;

void WorldCreator::load(const std::string& filepath)
{
    biomeObjects = wcil::WorldCreatorInstanceLoader::LoadInstanceInfo(filepath);
    // make sure we have unique asset names (not always the case in WC exports)
    std::set<std::string> existingNames;
    for (auto& biome : biomeObjects) {
        std::string originalName = biome.Name;
        int suffix = 1;
        while (existingNames.find(biome.Name) != existingNames.end()) {
            biome.Name = originalName + "_" + std::to_string(suffix);
            suffix++;
        }
        existingNames.insert(biome.Name);
    }
}

void WorldCreator::loadBiomeCSVData(const std::string& csvDir, wcil::BiomeObject& biome)
{
    wcil::WorldCreatorInstanceLoader::LoadParsedTilesFor(biome, csvDir);
}
