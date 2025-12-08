#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <charconv>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <mutex>

// JSON: add nlohmann/json.hpp to your project include path.
// https://github.com/nlohmann/json
#include <nlohmann/json.hpp>

// World Creator Instance Loader
namespace wcil
{
    struct Vec2 { float Min{}, Max{}; };
    struct Vec3 { float x{}, y{}, z{}; };
    struct Quat { float x{}, y{}, z{}, w{}; };

    struct MaterialInfo
    {
        std::string Name;
        float Roughness{};
        float Metalness{};
        std::string Color;           // RGBA hex "FFFFFFFF"
        std::string EmissionColor;   // RGBA hex "000000FF"
        float EmissionStrength{};
        bool UseGradient{};
        bool FlipNormalY{};
        float NormalStrength{};
        float RoughnessMapFactor{};
        float MetalnessMapFactor{};
        bool UsesGlossMap{};
        bool TerrainBlending{};
        Vec2 TerrainBlendingRange{};
    };

    struct RangeF { float Min{}, Max{}; };

    struct ObjectInfo
    {
        bool IsEntity{};
        std::string Path;
        float ModelScale{};
        std::string ModelYAxis;

        std::vector<MaterialInfo> Materials;

        float InstanceDistance{};
        float Density{};
        bool AlignToNormal{};
        float HeightOffset{};
        float NormalOffset{};
        bool UniformScaling{};

        RangeF ScaleRange{};
        RangeF ScaleRangeX{}, ScaleRangeY{}, ScaleRangeZ{};
        RangeF RotationRangeX{}, RotationRangeY{}, RotationRangeZ{};

        uint32_t Seed{};
        bool UseDistributionDensityRange{};
        RangeF DistributionDensityRange{};
        bool UseDistributionScaleRange{};
        RangeF DistributionScaleRange{};
        bool UseDistributionGradientRange{};
        RangeF DistributionGradientRange{};
        float DistributionGradientRandomness{};
    };

    struct InstanceTile
    {
        std::string FileName;   // CSV file name
        std::string FileType;   // "csv"
        bool IsEntity{};
        int TileX{};
        int TileY{};
        float TileSize{};
        int DataOffset{};       // line offset in CSV (0-based relative to first data line)
        int DataCount{};        // number of lines to read from offset
    };

    struct InstanceRecord
    {
        Vec3 t;     // tx,ty,tz
        Vec3 s;     // sx,sy,sz
        Quat q;     // qx,qy,qz,qw
        float gradient{};
        uint32_t seed{};
    };

    struct LoadedTileData
    {
        InstanceTile tile;
        std::vector<InstanceRecord> instances;
    };

    struct BiomeObject
    {
        std::string Biome;
        std::string Name;
        ObjectInfo Info;
        std::vector<InstanceTile> Tiles;

        // Parsed CSV instances per tile (filled by LoadParsedTilesFor)
        std::vector<LoadedTileData> ParsedTiles;

        // Optional: single merged tile containing all instances from ParsedTiles
        std::optional<LoadedTileData> MergedParsedTile;
    };

    class WorldCreatorInstanceLoader
    {
    public:
        // Parse forest_InstanceInfo.json
        static std::vector<BiomeObject> LoadInstanceInfo(const std::filesystem::path& jsonPath)
        {
            std::ifstream in(jsonPath, std::ios::binary);
            if (!in) throw std::runtime_error("Failed to open JSON: " + jsonPath.string());

            nlohmann::json j;
            in >> j;

            std::vector<BiomeObject> result;
            const auto& list = j.at("BiomeObjectList");

            for (const auto& e : list)
            {
                BiomeObject bo;
                bo.Biome = e.at("Biome").get<std::string>();
                bo.Name = e.at("Name").get<std::string>();

                const auto& oi = e.at("ObjectInfo");
                bo.Info.IsEntity = oi.at("IsEntity").get<bool>();
                bo.Info.Path = oi.at("Path").get<std::string>();
                bo.Info.ModelScale = oi.at("ModelScale").get<float>();
                bo.Info.ModelYAxis = oi.at("ModelYAxis").get<std::string>();

                // Materials
                if (oi.contains("MaterialInfos"))
                {
                    for (const auto& mi : oi.at("MaterialInfos"))
                    {
                        MaterialInfo m{};
                        m.Name = mi.at("Name").get<std::string>();
                        m.Roughness = mi.at("Roughness").get<float>();
                        m.Metalness = mi.at("Metalness").get<float>();
                        m.Color = mi.at("Color").get<std::string>();
                        m.EmissionColor = mi.at("EmissionColor").get<std::string>();
                        m.EmissionStrength = mi.at("EmissionStrength").get<float>();
                        m.UseGradient = mi.at("UseGradient").get<bool>();
                        m.FlipNormalY = mi.at("FlipNormalY").get<bool>();
                        m.NormalStrength = mi.at("NormalStrength").get<float>();
                        m.RoughnessMapFactor = mi.at("RoughnessMapFactor").get<float>();
                        m.MetalnessMapFactor = mi.at("MetalnessMapFactor").get<float>();
                        m.UsesGlossMap = mi.at("UsesGlossMap").get<bool>();
                        m.TerrainBlending = mi.at("TerrainBlending").get<bool>();
                        if (mi.contains("TerrainBlendingRange"))
                        {
                            m.TerrainBlendingRange.Min = mi["TerrainBlendingRange"].at("Min").get<float>();
                            m.TerrainBlendingRange.Max = mi["TerrainBlendingRange"].at("Max").get<float>();
                        }
                        bo.Info.Materials.emplace_back(std::move(m));
                    }
                }

                // Common numeric fields
                bo.Info.InstanceDistance = oi.at("InstanceDistance").get<float>();
                bo.Info.Density = oi.at("Density").get<float>();
                bo.Info.AlignToNormal = oi.at("AlignToNormal").get<bool>();
                bo.Info.HeightOffset = oi.at("HeightOffset").get<float>();
                bo.Info.NormalOffset = oi.at("NormalOffset").get<float>();
                bo.Info.UniformScaling = oi.at("UniformScaling").get<bool>();

                auto readRange = [](const nlohmann::json& node, const char* key, RangeF& out)
                {
                    if (node.contains(key))
                    {
                        out.Min = node.at(key).at("Min").get<float>();
                        out.Max = node.at(key).at("Max").get<float>();
                    }
                };

                readRange(oi, "ScaleRange", bo.Info.ScaleRange);
                readRange(oi, "ScaleRangeX", bo.Info.ScaleRangeX);
                readRange(oi, "ScaleRangeY", bo.Info.ScaleRangeY);
                readRange(oi, "ScaleRangeZ", bo.Info.ScaleRangeZ);
                readRange(oi, "RotationRangeX", bo.Info.RotationRangeX);
                readRange(oi, "RotationRangeY", bo.Info.RotationRangeY);
                readRange(oi, "RotationRangeZ", bo.Info.RotationRangeZ);

                bo.Info.Seed = oi.at("Seed").get<uint32_t>();
                bo.Info.UseDistributionDensityRange = oi.at("UseDistributionDensityRange").get<bool>();
                readRange(oi, "DistributionDensityRange", bo.Info.DistributionDensityRange);

                bo.Info.UseDistributionScaleRange = oi.at("UseDistributionScaleRange").get<bool>();
                readRange(oi, "DistributionScaleRange", bo.Info.DistributionScaleRange);

                bo.Info.UseDistributionGradientRange = oi.at("UseDistributionGradientRange").get<bool>();
                readRange(oi, "DistributionGradientRange", bo.Info.DistributionGradientRange);

                bo.Info.DistributionGradientRandomness = oi.at("DistributionGradientRandomness").get<float>();

                // Tiles
                if (e.contains("InstanceDataFiles"))
                {
                    for (const auto& tf : e.at("InstanceDataFiles"))
                    {
                        InstanceTile t{};
                        t.FileName = tf.at("FileName").get<std::string>();
                        t.FileType = tf.at("FileType").get<std::string>();
                        t.IsEntity = tf.at("IsEntity").get<bool>();
                        t.TileX = tf.at("TileX").get<int>();
                        t.TileY = tf.at("TileY").get<int>();
                        t.TileSize = tf.at("TileSize").get<float>();
                        t.DataOffset = tf.at("DataOffset").get<int>();
                        t.DataCount = tf.at("DataCount").get<int>();
                        bo.Tiles.emplace_back(std::move(t));
                    }
                }

                result.emplace_back(std::move(bo));
            }

            return result;
        }

        // Parse and attach CSV instances:
        // - If mergeIntoSingle == false: fills BiomeObject::ParsedTiles (per tile)
        // - If mergeIntoSingle == true: fills BiomeObject::MergedParsedTile (single merged tile)
        static void LoadParsedTilesFor(BiomeObject& bo, const std::filesystem::path& baseDir, bool mergeIntoSingle = true)
        {
            bo.ParsedTiles.clear();
            bo.MergedParsedTile.reset();

            if (!mergeIntoSingle)
            {
                for (const auto& t : bo.Tiles)
                {
                    if (t.DataCount == 0) continue;
                    if (!EqualNoCase(t.FileType, "csv")) continue;

                    LoadedTileData ltd;
                    ltd.tile = t;

                    const auto path = baseDir / t.FileName;
                    ltd.instances = LoadTileCsv(path, t.DataOffset, t.DataCount);

                    if (!ltd.instances.empty())
                        bo.ParsedTiles.emplace_back(std::move(ltd));
                }
                return;
            }

            // Merge mode: concatenate all instances into one tile
            LoadedTileData merged{};
            merged.tile.FileName = "merged";
            merged.tile.FileType = "csv";
            merged.tile.IsEntity = false;
            merged.tile.TileX = 0;
            merged.tile.TileY = 0;
            merged.tile.TileSize = 0.0f;
            merged.tile.DataOffset = 0;
            merged.tile.DataCount = 0;

            // Precompute total size to reserve once
            size_t total = 0;
            for (const auto& t : bo.Tiles)
            {
                if (t.DataCount == 0) continue;
                if (!EqualNoCase(t.FileType, "csv")) continue;

                const auto path = baseDir / t.FileName;
                const auto& lines = ReadCsvLinesCached(path);

                int startIndex = IsHeaderLine(lines.empty() ? std::string{} : lines[0]) ? 1 : 0;
                startIndex += std::max(0, t.DataOffset);
                int endIndex = (t.DataCount > 0)
                    ? std::min<int>(startIndex + t.DataCount, static_cast<int>(lines.size()))
                    : static_cast<int>(lines.size());
                if (endIndex > startIndex)
                    total += static_cast<size_t>(endIndex - startIndex);
            }
            merged.instances.reserve(total);

            // Append instances
            for (const auto& t : bo.Tiles)
            {
                if (t.DataCount == 0) continue;
                if (!EqualNoCase(t.FileType, "csv")) continue;

                const auto path = baseDir / t.FileName;
                const auto instances = LoadTileCsv(path, t.DataOffset, t.DataCount);
                if (!instances.empty())
                {
                    merged.instances.insert(merged.instances.end(), instances.begin(), instances.end());
                }
            }

            // Set merged tile (only if we actually collected instances)
            if (!merged.instances.empty())
                bo.MergedParsedTile = std::move(merged);
        }

        // Optional: clear whole CSV cache (e.g., when reloading a scene)
        static void ClearCsvCache()
        {
            std::scoped_lock lock(s_cacheMutex);
            s_csvCache.clear();
        }

    private:
        // Load and parse a slice of a CSV using cache
        static std::vector<InstanceRecord> LoadTileCsv(
            const std::filesystem::path& csvPath,
            int dataOffset,
            int dataCount)
        {
            const auto& lines = ReadCsvLinesCached(csvPath);
            if (lines.empty()) return {};

            // Detect header
            int startIndex = 0;
            if (IsHeaderLine(lines[0]))
                startIndex = 1;

            // Apply DataOffset relative to the first data row
            startIndex += std::max(0, dataOffset);

            int endIndex = (dataCount > 0)
                ? std::min<int>(startIndex + dataCount, static_cast<int>(lines.size()))
                : static_cast<int>(lines.size());

            std::vector<InstanceRecord> out;
            out.reserve(std::max(0, endIndex - startIndex));

            for (int i = startIndex; i < endIndex; ++i)
            {
                InstanceRecord rec{};
                if (ParseCsvRow(lines[i], rec))
                    out.emplace_back(rec);
            }
            return out;
        }

        // CSV cache: path -> lines
        static const std::vector<std::string>& ReadCsvLinesCached(const std::filesystem::path& csvPath)
        {
            const std::string key = csvPath.generic_string();

            {
                std::scoped_lock lock(s_cacheMutex);
                auto it = s_csvCache.find(key);
                if (it != s_csvCache.end())
                {
                    return it->second;
                }
            }

            // Load file outside the lock to avoid I/O under mutex
            std::vector<std::string> lines;
            {
                std::ifstream in(csvPath, std::ios::binary);
                if (!in) throw std::runtime_error("Failed to open CSV: " + csvPath.string());

                std::string line;
                lines.reserve(1024);
                while (std::getline(in, line))
                {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    lines.emplace_back(std::move(line));
                }
            }

            // Store in cache
            std::scoped_lock lock2(s_cacheMutex);
            auto [it, inserted] = s_csvCache.emplace(key, std::move(lines));
            return it->second;
        }

        static bool IsHeaderLine(const std::string& s)
        {
            // Expect: tx,ty,tz,sx,sy,sz,qx,qy,qz,qw,gradient,seed
            return s.find("tx,ty,tz") != std::string::npos && s.find(",seed") != std::string::npos;
        }

        static bool EqualNoCase(const std::string& a, const std::string& b)
        {
            return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                              [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
        }

        static bool ParseCsvRow(const std::string& row, InstanceRecord& out)
        {
            float values[11];
            uint32_t seedVal = 0;

            std::vector<std::string_view> cols;
            cols.reserve(16);
            const char* begin = row.c_str();
            const char* cur = begin;
            const char* end = begin + row.size();

            while (cur <= end)
            {
                const char* comma = static_cast<const char*>(memchr(cur, ',', static_cast<size_t>(end - cur)));
                if (!comma)
                {
                    cols.emplace_back(std::string_view(cur, static_cast<size_t>(end - cur)));
                    break;
                }
                cols.emplace_back(std::string_view(cur, static_cast<size_t>(comma - cur)));
                cur = comma + 1;
            }

            if (cols.size() != 12) return false; // expected 12 columns

            for (int i = 0; i < 11; ++i)
            {
                float f{};
                if (!ParseFloat(cols[i], f)) return false;
                values[i] = f;
            }
            if (!ParseUint(cols[11], seedVal)) return false;

            out.t = { values[0], values[1], values[2] };
            out.s = { values[3], values[4], values[5] };
            out.q = { values[6], values[7], values[8], values[9] };
            out.gradient = values[10];
            out.seed = seedVal;
            return true;
        }

        static bool ParseFloat(std::string_view sv, float& out)
        {
            const char* first = sv.data();
            const char* last  = sv.data() + sv.size();
            auto res = std::from_chars(first, last, out);
            if (res.ec == std::errc::invalid_argument || res.ptr != last)
            {
                try {
                    out = std::stof(std::string(sv));
                    return true;
                } catch (...) { return false; }
            }
            return res.ec == std::errc();
        }

        static bool ParseUint(std::string_view sv, uint32_t& out)
        {
            const char* first = sv.data();
            const char* last  = sv.data() + sv.size();
            auto res = std::from_chars(first, last, out, 10);
            return res.ec == std::errc() && res.ptr == last;
        }

        inline static std::unordered_map<std::string, std::vector<std::string>> s_csvCache{};
        inline static std::mutex s_cacheMutex{};
    };
}