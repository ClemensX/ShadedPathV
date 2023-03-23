#pragma once
class ShadedPathEngine;
class ThreadResources;
struct LineDef;
class World;

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
    static void printCStringList(std::vector<const char*>& exts) {
        for (uint32_t i = 0; i < exts.size(); i++) {
            Log("  " << exts[i] << std::endl);
        }
    };
    static void printMatrix (glm::mat4 m) {
        Log("matrix 1.row: " << m[0][0] << " " << m[0][1] << " " << m[0][2] << " " << m[0][3] << std::endl);
        Log("matrix 2.row: " << m[1][0] << " " << m[1][1] << " " << m[1][2] << " " << m[1][3] << std::endl);
        Log("matrix 3.row: " << m[2][0] << " " << m[2][1] << " " << m[2][2] << " " << m[2][3] << std::endl);
        Log("matrix 4.row: " << m[3][0] << " " << m[3][1] << " " << m[3][2] << " " << m[3][3] << std::endl);
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

    // create debug name for thread resource object from name and ThreadResource id
    std::string createDebugName(const char* name, ThreadResources& res);

    // name vulkan objects. used to identify them in debug message from validation layer
    // general purpost method that can be used foe all object types
    void debugNameObject(uint64_t object, VkObjectType objectType, const char* name);

    // debug name command buffers
    void debugNameObjectCommandBuffer(VkCommandBuffer cbuf, const char* name) {
        debugNameObject((uint64_t)cbuf, VK_OBJECT_TYPE_COMMAND_BUFFER, name);
    }
    // debug name command buffers
    void debugNameObjectBuffer(VkBuffer buf, const char* name) {
        debugNameObject((uint64_t)buf, VK_OBJECT_TYPE_BUFFER, name);
    }
    // debug name command buffers
    void debugNameObjectDeviceMmeory(VkDeviceMemory m, const char* name) {
        debugNameObject((uint64_t)m, VK_OBJECT_TYPE_DEVICE_MEMORY, name);
    }
    // debug name descriptor set layout
    void debugNameObjectDescriptorSetLayout(VkDescriptorSetLayout m, const char* name) {
        debugNameObject((uint64_t)m, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name);
    }
    // debug name descriptor set
    void debugNameObjectDescriptorSet(VkDescriptorSet m, const char* name) {
        debugNameObject((uint64_t)m, VK_OBJECT_TYPE_DESCRIPTOR_SET, name);
    }
    // debug name shader module
    void debugNameObjectShaderModule(VkShaderModule m, const char* name) {
        debugNameObject((uint64_t)m, VK_OBJECT_TYPE_SHADER_MODULE, name);
    }

private:
    void initializeDebugFunctionPointers();
    //PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectNameEXT = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT pfnDebugUtilsObjectNameEXT = nullptr;
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

    // Returns random float in [0, 1).
    static float RandF()
    {
        return (float)(rand()) / (float)RAND_MAX;
    }

    // Returns random float in [a, b).
    static float RandF(float a, float b)
    {
        return a + RandF() * (b - a);
    }
};

// Spatial data for creating hightmap, tailored for Diamond-square algorithm
// all points are NAN initially, only non-NAN values will be considered valid
// only after very last diamond-square step all NANs will be gone
// https://en.wikipedia.org/wiki/Diamond-square_algorithm
class Spatial2D {
public:
    Spatial2D(int N2plus1);

    // total number of elements
    int size() {
        return heightmap_size;
    }
    // set height of one point
    void setHeight(int x, int y, float height);

    // get all lines of current heightmap. Used to draw the current iteration with simple lines.
    // all NAN points will be omitted, but still may form a line: p1--NAN--NAN--NAN--p2 will draw line p1--p2
    void getLines(std::vector<LineDef> &lines);
    // get all points of heightmap as vec3
    void getPoints(std::vector<glm::vec3>& points);
    // recalculate lines from 0-max index to world coordinates
    void adaptLinesToWorld(std::vector<LineDef>& lines, World &world);
    // perform diamond-square. Corner points need to have been set before calling this.
    // number of steps can be specified. 0 is only setup points, -1 is all steps
    // heightmap line 0 is at (-halfsize, h, -halfsize) to (halfsize, h, -halfsize)
    // randomMagnitude is initial range of random value added to calculated points
    // RandomDampening is a factor by which the dampeningMagnitude will be made smaller after each step (1 .. 0.933)
    void diamondSquare(float randomMagnitude, float randomDampening, int steps = -1);
    // check that every point is set (no more NAN elements). This means we have a full heightmap
    bool isAllPointsSet();

private:
    int sidePoints = 0;
    float* h = nullptr;

    // return index into 1 dimensional height array from coords:
    int index(int x, int y);
    // one step of diamond square. squareWidth is segments, not points!
    void diamondSquareOneStep(float randomMagnitude, float randomDampening, int squareWidth);
    void diamondStep(float randomMagnitude, float randomDampening, int squareWidth);
    void squareStep(float randomMagnitude, float randomDampening, int squareWidth);
    float getAverageDiamond(int center_x, int center_y, int half);
    float getAverageSquare(int center_x, int center_y, int half);
    // return height value, NAN if outside boundaries
    float getHeightSave(int center_x, int center_y, int half);
    int heightmap_size = 0;
};
