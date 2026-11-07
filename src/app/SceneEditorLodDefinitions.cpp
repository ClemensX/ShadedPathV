#include "mainheader.h"
#include "SceneEditorLodDefinitions.h"
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>

namespace {
std::string cppStringLiteral(const std::string& value)
{
    std::string result = "\"";
    for (unsigned char ch : value) {
        if (ch == '"' || ch == '\\') {
            result += '\\';
            result += static_cast<char>(ch);
        }
        else if (ch >= 32 && ch <= 126) {
            result += static_cast<char>(ch);
        }
        else {
            result += '\\';
            result += static_cast<char>('0' + ((ch >> 6) & 7));
            result += static_cast<char>('0' + ((ch >> 3) & 7));
            result += static_cast<char>('0' + (ch & 7));
        }
    }
    return result + '"';
}

std::string categoryMacroName(const std::string& name)
{
    std::string result = "LOD_CATEGORY_";
    for (unsigned char ch : name) {
        if (ch >= 'a' && ch <= 'z') {
            result += static_cast<char>(ch - 'a' + 'A');
        }
        else if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_') {
            result += static_cast<char>(ch);
        }
        else {
            result += '_';
        }
    }
    return result;
}

std::string cppFloatLiteral(float value)
{
    if (std::isnan(value)) {
        return "std::numeric_limits<float>::quiet_NaN()";
    }
    if (std::isinf(value)) {
        return value < 0 ? "-std::numeric_limits<float>::infinity()" : "std::numeric_limits<float>::infinity()";
    }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
    std::string result = out.str();
    if (result.find_first_of(".eE") == std::string::npos) {
        result += ".0";
    }
    return result + 'f';
}
}

std::string formatLodDefinitions(const std::vector<SceneEditorLodDefinition>& definitions)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    std::vector<const SceneEditorLodDefinition*> sortedCategories;
    for (const auto& definition : definitions) {
        sortedCategories.push_back(&definition);
    }
    std::stable_sort(sortedCategories.begin(), sortedCategories.end(), [](const auto* a, const auto* b) {
        return a->id < b->id;
    });
    std::unordered_set<std::string> macroNames;
    for (const auto* definition : sortedCategories) {
        const std::string macroName = categoryMacroName(definition->name);
        if (!macroNames.insert(macroName).second) {
            out << "#error Duplicate LOD category macro " << macroName << "; rename the category\n";
        }
        out << "#define " << macroName << " " << definition->id << "\n";
    }
    out << "\ninline constexpr LodCategoryDef LOD_CATEGORIES[] = {\n";
    for (const auto* definition : sortedCategories) {
        out << "    { " << categoryMacroName(definition->name) << ", " << cppStringLiteral(definition->name) << " },\n";
    }
    out << "};\n\n";

    const auto count = std::count_if(definitions.begin(), definitions.end(), [](const auto& definition) {
        return definition.distances.has_value();
    });
    out << "#define LOD_ENTRIES_COUNT " << count << "\n\n";
    out << "LodEntry LOD_ENTRIES[LOD_ENTRIES_COUNT] = {\n";
    for (const auto& definition : definitions) {
        if (!definition.distances) {
            continue;
        }
        const LodEntry& entry = *definition.distances;
        out << "    { " << entry.category << ", " << entry.max_index_used << ", "
            << cppFloatLiteral(entry.minDistance) << ", " << cppFloatLiteral(entry.maxDistance) << ", { ";
        for (size_t i = 0; i < std::size(entry.dist_map); ++i) {
            if (i != 0) {
                out << ", ";
            }
            out << cppFloatLiteral(entry.dist_map[i]);
        }
        out << " } },\n";
    }
    out << "};\n";
    return out.str();
}
