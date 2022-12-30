#pragma once
class ShadedPathEngine;

namespace Colors {
    const glm::vec4 xm{ 1.0f, 0.0f, 1.0f, 1.0f };
    const glm::vec4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
    const glm::vec4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
    const glm::vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
    const glm::vec4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
    const glm::vec4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
    const glm::vec4 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
    const glm::vec4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
    const glm::vec4 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };

    const glm::vec4 Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
    const glm::vec4 LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };
};

class Util
{
public:
    // if you construct a skybox you need to be sure the edges of the view cube are still within far plane.
    // this calculates the maximum (half) cube edge size you can use (see CubeShader)
    static float getMaxCubeViewDistanceFromFarPlane(float f) {
        return sqrt((f * f) / 3.0f);
    }
    static void printCStringList(std::vector<const char *> &exts) {
        for (uint32_t i = 0; i < exts.size(); i++) {
            Log("  " << exts[i] << std::endl);
        }
    };
    static std::string decodeVulkanVersion(uint32_t version) {
        std::string s;
        s = s + ("") + std::to_string(VK_API_VERSION_MAJOR(version)) + "." + std::to_string(VK_API_VERSION_MINOR(version)) + "." + std::to_string(VK_API_VERSION_PATCH(version));
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

    Util(ShadedPathEngine& s) {
        Log("Util c'tor\n");
        engine = &s;
    };

    ~Util() {};

    // name vulkan objects. used to identify them in debug message from validation layer
    // general purpost method that can be used foe all object types
    void debugNameObject(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name);

    // debug name command buffers
    void debugNameObjectCommandBuffer(VkCommandBuffer cbuf, const char* name) {
        debugNameObject((uint64_t)cbuf, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, name);
    }
    // debug name command buffers
    void debugNameObjectBuffer(VkBuffer buf, const char* name) {
        debugNameObject((uint64_t)buf, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, name);
    }
    // debug name command buffers
    void debugNameObjectDeviceMmeory(VkDeviceMemory m, const char* name) {
        debugNameObject((uint64_t)m, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, name);
    }
    // debug name descriptor set layout
    void debugNameObjectDescriptorSetLayout(VkDescriptorSetLayout m, const char* name) {
        debugNameObject((uint64_t)m, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, name);
    }
    // debug name descriptor set
    void debugNameObjectDescriptorSet(VkDescriptorSet m, const char* name) {
        debugNameObject((uint64_t)m, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, name);
    }
    // debug name shader module
    void debugNameObjectShaderModule(VkShaderModule m, const char* name) {
        debugNameObject((uint64_t)m, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, name);
    }

private:
    void initializeDebugFunctionPointers();
    PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectNameEXT = nullptr;
    ShadedPathEngine* engine = nullptr;
};

class LogfileScanner
{
public:
    LogfileScanner();
    bool assertLineBefore(std::string before, std::string after);
    // search for line in logfile and return line number, -1 means not found
    // matching line beginnings is enough
    int searchForLine(std::string line, int startline = 0);
private:
    std::vector<std::string> lines;
};

struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
};

// https://github.com/opengl-tutorials/ogl/blob/master/common/quaternion_utils.cpp
class MathHelper {
public:
    // Returns a quaternion such that q*start = dest
    static glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest);

    // Returns a quaternion that will make your object looking towards 'direction'.
    // Similar to RotationBetweenVectors, but also controls the vertical orientation.
    // This assumes that at rest, the object faces +Z.
    // Beware, the first parameter is a direction, not the target point !
    static glm::quat LookAt(glm::vec3 direction, glm::vec3 desiredUp);
};
