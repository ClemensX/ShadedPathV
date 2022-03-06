#pragma once
namespace Colors {
    const vec4 xm{ 1.0f, 0.0f, 1.0f, 1.0f };
    const vec4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
    const vec4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
    const vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
    const vec4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
    const vec4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
    const vec4 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
    const vec4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
    const vec4 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };

    const vec4 Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
    const vec4 LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };
};

class Util
{
public:
    static void printCStringList(vector<const char *> &exts) {
        for (uint32_t i = 0; i < exts.size(); i++) {
            Log("  " << exts[i] << endl);
        }
    };
    static string decodeVulkanVersion(uint32_t version) {
        string s;
        s = s + ("") + to_string(VK_API_VERSION_MAJOR(version)) + "." + to_string(VK_API_VERSION_MINOR(version)) + "." + to_string(VK_API_VERSION_PATCH(version));
        return s;
    }
    static const char* decodeDeviceType(uint32_t type) {
        switch (type) {
        case 0: return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
        case 1: return "INTEGRATED_GPU";
        case 2: return "DISCRETE_GPU";
        case 3: return "VIRTUAL_GPU";
        case 4: return "CPU";
        default: return "n/a";
        };
    }
};

class LogfileScanner
{
public:
    LogfileScanner();
    bool assertLineBefore(string before, string after);
    // search for line in logfile and return line number, -1 means not found
    // matching line beginnings is enough
    int searchForLine(string line, int startline = 0);
private:
    vector<string> lines;
};

struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
};

