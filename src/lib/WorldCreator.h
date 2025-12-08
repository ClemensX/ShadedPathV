#pragma once

// handle WorldCreator data.
// this is a leightweight addition to ShadedPath engine, meaning this utility class
// offers stand-alone functionality and has no connections/requirements to other ShadedPath classes
class WorldCreator
{
public:
    // Load world creator instance info json file from the specified file path.
    void load(const std::string& filepath);
    // biomes with objects
    std::vector<wcil::BiomeObject> biomeObjects;
    // load CSV data for single biome
    void loadBiomeCSVData(const std::string& csvFilePath, wcil::BiomeObject& biome);
};
