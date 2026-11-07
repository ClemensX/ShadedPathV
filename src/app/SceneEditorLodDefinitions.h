#pragma once

struct SceneEditorLodDefinition
{
    int id;
    std::string name;
    std::optional<LodEntry> distances;
};

std::string formatLodDefinitions(const std::vector<SceneEditorLodDefinition>& definitions);
