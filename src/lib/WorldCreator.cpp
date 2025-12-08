#include "mainheader.h"
#include "WorldCreator.h"

using namespace glm;

void WorldCreator::load(const std::string& filepath)
{
    biomeObjects = wcil::WorldCreatorInstanceLoader::LoadInstanceInfo(filepath);
}

void WorldCreator::loadBiomeCSVData(const std::string& csvDir, wcil::BiomeObject& biome)
{
    wcil::WorldCreatorInstanceLoader::LoadParsedTilesFor(biome, csvDir);
}
