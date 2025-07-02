#include "mainheader.h"

using namespace std;


/* Put this in a single .cpp file that's vulkan related: */
PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT_ = nullptr;

LogfileScanner::LogfileScanner()
{
    string line;
    ifstream myfile("spe_run.log");
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            lines.push_back(line);
        }
        myfile.close();
        //Log("lines: " << lines.size() << endl);
    }
    else {
        Error("Could not open log file");
    }
}

bool LogfileScanner::assertLineBefore(string before, string after)
{
    int beforePos = searchForLine(before);
    if (beforePos < 0)
        return false;
    int afterPos = searchForLine(after, beforePos);
    if (beforePos < afterPos)
        return true;
    return false;
}

int LogfileScanner::searchForLine(string line, int startline)
{
    for (int i = startline; i < lines.size(); i++) {
        string lineInFile = lines.at(i);
        if (lineInFile.rfind(line, 0) != string::npos)
            return i;
    }
    return -1;
}

void Util::initializeDebugFunctionPointers() {
        pfnDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(engine->globalRendering.vkInstance, "vkSetDebugUtilsObjectNameEXT");
        /* Put this in your code that initializes Vulkan (after you create your VkInstance and VkDevice): */
        vkCmdDrawMeshTasksEXT_ = (PFN_vkCmdDrawMeshTasksEXT)vkGetInstanceProcAddr(engine->globalRendering.vkInstance, "vkCmdDrawMeshTasksEXT");
}

std::string Util::createDebugName(const char* name, int number) {
    return std::string(name) + "_" + std::to_string(number);
}

// vulkan will copy the name string, so we can use a temporary string
void Util::debugNameObject(uint64_t object, VkObjectType objectType, const char* name) {
    if (pfnDebugUtilsObjectNameEXT == nullptr) {
        initializeDebugFunctionPointers();
    }
    // Check for a valid function pointer
    if (pfnDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = object;
        nameInfo.pObjectName = name;
        pfnDebugUtilsObjectNameEXT(engine->globalRendering.device, &nameInfo);
    }
}

void Util::warn(std::string msg)
{
    if (msg != last_warn_msg) {
        Log("WARNING: " << msg << endl);
        last_warn_msg = msg;
    }
}

// MathHelper:
using namespace glm;

glm::quat MathHelper::RotationBetweenVectors(glm::vec3 start, glm::vec3 dest)
{
    start = normalize(start);
    dest = normalize(dest);

    float cosTheta = dot(start, dest);
    vec3 rotationAxis;

    if (cosTheta < -1 + 0.001f) {
        // special case when vectors in opposite directions :
        // there is no "ideal" rotation axis
        // So guess one; any will do as long as it's perpendicular to start
        // This implementation favors a rotation around the Up axis,
        // since it's often what you want to do.
        rotationAxis = cross(vec3(0.0f, 0.0f, 1.0f), start);
        if (length2(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
            rotationAxis = cross(vec3(1.0f, 0.0f, 0.0f), start);

        rotationAxis = normalize(rotationAxis);
        return angleAxis(glm::radians(180.0f), rotationAxis);
    }

    // Implementation from Stan Melax's Game Programming Gems 1 article
    rotationAxis = cross(start, dest);

    float s = sqrt((1 + cosTheta) * 2);
    float invs = 1 / s;

    return quat(
        s * 0.5f,
        rotationAxis.x * invs,
        rotationAxis.y * invs,
        rotationAxis.z * invs
    );
}

glm::quat MathHelper::LookAt(glm::vec3 direction, glm::vec3 desiredUp)
{

    if (length2(direction) < 0.0001f)
        return quat();

    // Recompute desiredUp so that it's perpendicular to the direction
    // You can skip that part if you really want to force desiredUp
    vec3 right = cross(direction, desiredUp);
    desiredUp = cross(right, direction);

    // Find the rotation between the front of the object (that we assume towards +Z,
    // but this depends on your model) and the desired direction
    quat rot1 = RotationBetweenVectors(vec3(0.0f, 0.0f, 1.0f), direction);
    // Because of the 1rst rotation, the up is probably completely screwed up. 
    // Find the rotation between the "up" of the rotated object, and the desired up
    vec3 newUp = rot1 * vec3(0.0f, 1.0f, 0.0f);
    quat rot2 = RotationBetweenVectors(newUp, desiredUp);

    // Apply them
    return rot2 * rot1; // remember, in reverse order.
}

void MathHelper::getAxisAngleFromQuaternion(const glm::quat& q, glm::vec3& axis, float& angle) {
    // Ensure the quaternion is normalized
    glm::quat normalizedQ = glm::normalize(q);

    // Calculate the angle
    angle = 2.0f * acos(normalizedQ.w);

    // Calculate the axis
    float s = sqrt(1.0f - normalizedQ.w * normalizedQ.w);
    if (s < 0.001f) { // To avoid division by zero
        axis = glm::vec3(1.0f, 0.0f, 0.0f); // Arbitrary axis
    }
    else {
        axis = glm::vec3(normalizedQ.x / s, normalizedQ.y / s, normalizedQ.z / s);
    }
}

Spatial2D::Spatial2D(int N2plus1)
{
    resetSize(N2plus1);
}

void Spatial2D::resetSize(int N2plus1) {
    // make sure we have a side length of form 2 * n + 1:
    double s = log2((double)(N2plus1 - 1));
    int n = (int)s;
    int n2 = 2 << (n - 1);
    if (n2 != (N2plus1 - 1)) {
        throw std::out_of_range("Spatial2D needs side length initializer of the form (2 ^ N) + 1");
    }
    if (sidePoints != N2plus1) {
        h = new float[N2plus1 * N2plus1];
        sidePoints = N2plus1;
    }
    for (int i = 0; i < N2plus1 * N2plus1; i++) {
        h[i] = NAN;
    }
    heightmap_size = N2plus1 * N2plus1;
}


int Spatial2D::index(int x, int y)
{
    return y * sidePoints + x;
}

void Spatial2D::setHeight(int x, int y, float height)
{
    int i = index(x, y);
    if (i >= heightmap_size) {
        throw std::out_of_range("Spacial2D tried to set point outside range of heightmap!");
    }
    h[i] = height;
}

void Spatial2D::getLines(vector<LineDef> &lines)
{
    // horizontal lines:
    LineDef line;
    line.color = Colors::Red;
    int p0 = -1, p1 = -1;
    for (int y = 0; y < sidePoints; y++) {
        for (int x = 0; x < sidePoints; x++) {
            // find p0
            float h1 = h[index(x, y)];
            if (!std::isnan(h1)) {
                // valid point found
                if (p0 == -1) {
                    // start point found
                    p0 = x;
                }
                else {
                    // end point found
                    float p0height = h[index(p0, y)];
                    p1 = x;
                    float p1height = h1;
                    line.start = vec3(p0, p0height, y);
                    line.end = vec3(p1, p1height, y);
                    lines.push_back(line);
                    p0 = p1; // end point is also new start point
                    p1 = -1;
                }
            }
        }
        p0 = -1;
    }
    // vertical lines:
    p0 = p1 = -1;
    for (int x = 0; x < sidePoints; x++) {
        for (int y = 0; y < sidePoints; y++) {
            // find p0
            float h1 = h[index(x, y)];
            if (!std::isnan(h1)) {
                // valid point found
                if (p0 == -1) {
                    // start point found
                    p0 = y;
                }
                else {
                    // end point found
                    float p0height = h[index(x, p0)];
                    p1 = y;
                    float p1height = h1;
                    line.start = vec3(x, p0height, p0);
                    line.end = vec3(x, p1height, p1);
                    lines.push_back(line);
                    p0 = p1; // end point is also new start point
                    p1 = -1;
                }
            }
        }
        p0 = -1;
    }
}

void Spatial2D::getPoints(std::vector<glm::vec3>& points)
{
    for (int y = 0; y < sidePoints; y++) {
        for (int x = 0; x < sidePoints; x++) {
            float height = h[index(x, y)];
            if (!std::isnan(height)) {
                // found point, converto to vec3 and add to list
                vec3 p(x, height, y);
                points.push_back(p);
                //Log("  p " << p.x << " " << p.z  << " height: " << p.y << endl);
            }
        }
    }
}

bool Spatial2D::isAllPointsSet()
{
    int totalSize = size();
    for (int i = 0; i < totalSize; i++) {
        if (std::isnan(h[i])) {
            return false;
        }
    }
    return true;
}

void Spatial2D::adaptLinesToWorld(std::vector<LineDef>& lines, World& world)
{
    vec4 center = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    vec3 worldSize = world.getWorldSize();
    float depth = worldSize.z;
    float width = worldSize.x;
    float halfDepth = depth / 2.0f;
    float halfWidth = width / 2.0f;
    int segments = sidePoints - 1;
    int depthCells = (int)(depth / segments);
    int widthCells = (int)(width / segments);

    // width/height if one segment in world x direction
    double indexXfactor = (double)width / (double)segments;
    double indexZfactor = (double)depth / (double)segments;
    double startx = -halfWidth;
    double startz = -halfDepth;
    //Log("index matching:" << endl);
    //Log("    from x: " << startx << " to " << (startx + (segments*indexXfactor)) << endl);
    for (LineDef& l : lines) {
        l.start.x = static_cast<float>(startx + l.start.x * indexXfactor);
        l.start.z = -1.0f * static_cast<float>(startz + l.start.z * indexXfactor);
        l.end.x = static_cast<float>(startx + l.end.x * indexXfactor);
        l.end.z = -1.0f * static_cast<float>(startz + l.end.z * indexZfactor);
    }
}

void Spatial2D::diamondSquare(float randomMagnitude, float randomDampening, int seed, int steps)
{
    if (steps == 0) {
        // nothing to do - simply return the corner points
        return;
    }
    if (seed != 0) {
        srand(seed);
    }
    if (randomMagnitude < 0.1f) {
        Log("ERROR: diamondSquare() needs positive random range" << endl);
        return;
    }
    //if (randomDampening < 0.932f || randomDampening > 1.0f) {
    //    Log("WARNING: diamondSquare() randomDampening should be within [0.933 , 1] << endl");
    //}
    int segments = sidePoints - 1;
    int max_steps = (int)log2((double)segments);
    if (steps == -1 || steps > max_steps) {
        steps = max_steps;
    }
    int circle_step_width = sidePoints - 1;
    for (int i = 0; i < steps; i++) {
        diamondSquareOneStep(randomMagnitude, randomDampening, circle_step_width);
        //vector<vec3> pts;
        //this->getPoints(pts);
        //Log(" Diamond Square side lenght " << (circle_step_width+1) << " has points: " << pts.size() << endl);
        randomMagnitude *= randomDampening;
        circle_step_width /= 2;
    }
}

void Spatial2D::diamondSquareOneStep(float randomMagnitude, float randomDampening, int squareWidth)
{
    diamondStep(randomMagnitude, randomDampening, squareWidth);
    squareStep(randomMagnitude, randomDampening, squareWidth);
}

void Spatial2D::diamondStep(float randomMagnitude, float randomDampening, int squareWidth)
{
    // iterate over all squares, because of n ^ 2 we will always have same boundaries in x and y direction
    int iterations_one_axis = (sidePoints - 1) / squareWidth;
    assert(iterations_one_axis * squareWidth == (sidePoints - 1));
    int half = squareWidth / 2;

    // iterate over x and y
    for (int xs = 0; xs < iterations_one_axis; xs++) {
        for (int ys = 0; ys < iterations_one_axis; ys++) {
            // we do this for all squares:
            // calc center coords:
            int center_x = xs * squareWidth + half;
            int center_y = ys * squareWidth + half;
            assert(std::isnan(h[index(center_x, center_y)]));
            float height = getAverageDiamond(center_x, center_y, half);
            float r = MathHelper::RandF(-randomMagnitude, randomMagnitude);
            height += r;
            h[index(center_x, center_y)] = height;
        }
    }
}

void Spatial2D::squareStep(float randomMagnitude, float randomDampening, int squareWidth)
{
    // iterate over all rows via half squareWidth, add one half to x for all even rows
    int half = squareWidth / 2;
    int row, col;
    row = col = 0;
    bool even = true;
    while (row < sidePoints) {
        col = even ? half : 0;
        while (col < sidePoints) {
            assert(std::isnan(h[index(col, row)]));
            float height = getAverageSquare(col, row, half);
            float r = MathHelper::RandF(-randomMagnitude, randomMagnitude);
            height += r;
            h[index(col, row)] = height;
            col += squareWidth;
        }
        row += half;
        even = !even;
    }
}

float Spatial2D::getAverageDiamond(int cx, int cy, int half)
{
    // calc top left/right and bottom lef/right corner values, none should be NAN
    // we think of y = 0 as bottom
    float bl = h[index(cx - half, cy - half)];
    float br = h[index(cx + half, cy - half)];
    float tl = h[index(cx - half, cy + half)];
    float tr = h[index(cx + half, cy + half)];
    assert(!std::isnan(bl));
    assert(!std::isnan(br));
    assert(!std::isnan(tl));
    assert(!std::isnan(tr));
    return (tl + tr + bl + br) / 4.0f;
}

float Spatial2D::getAverageSquare(int cx, int cy, int half)
{
    // calc left/right/bottom/left from center
    // we think of y = 0 as bottom
    //Log("square:  " << cx << " " << cy << endl);
    float div = 0.0f;
    float add = 0.0f;
    // left point
    float h = getHeightSave(cx - half, cy, half);
    if (!std::isnan(h)) {
        div += 1.0f;
        add += h;
    }
    // top point
    h = getHeightSave(cx, cy + half, half);
    if (!std::isnan(h)) {
        div += 1.0f;
        add += h;
    }
    // right points
    h = getHeightSave(cx + half, cy, half);
    if (!std::isnan(h)) {
        div += 1.0f;
        add += h;
    }
    // bottom point
    h = getHeightSave(cx, cy - half, half);
    if (!std::isnan(h)) {
        div += 1.0f;
        add += h;
    }
    return add/div;
}

float Spatial2D::getHeightSave(int center_x, int center_y, int half)
{
    if (center_x < 0 || center_x >= sidePoints ||
        center_y < 0 || center_y >= sidePoints) {
        return NAN;
    }
    float height = h[index(center_x, center_y)];
    return height;
}

void Util::writeRawImageTestData(GPUImage& img, int type)
{
    //for (int i = 0; i < 100000; i++) {
    //    unsigned int* row = (unsigned int*)(img.imagedata + i);
    //    if ((*row % 3) == 0) *((char*)row) = 0x3f;
    //}
    //unsigned char data[] = {0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff };
    //unsigned int* row = (unsigned int*)(img.imagedata);
    //for (int i = 0; i < 12; i++) {
    //    *((char*)row + i) = data[i];
    //}

    //writeRawImagePixel(img, 0, 0, Colors::Cyan);
    //writeRawImagePixel(img, 1, 0, Colors::LightSteelBlue);
    //writeRawImagePixel(img, 2, 2, Colors::Blue);
    //writeRawImagePixel(img, 3, 2, Colors::Magenta);
    static long count = 0;
    long mod = count++ % 1000;
    float modf = 0.001f * mod;
    //Log("b = " << modf << endl)
    if (type == 0 || type == 2) {
        // write all image pixels with color gradient
        float dist = (float)1.0 / img.width;
        for (int y = 0; y < img.height; y++) {
            for (int x = 0; x < img.width; x++) {
                float r = (float)x * dist;
                float g = (float)y * dist;
                float b = 0.0f;
                if (type == 2) {
                    b = modf;
                }
                float a = 255.0f;
                writeRawImagePixel(img, x, y, glm::vec4(r, g, b, a));
                if (x == y) {
                    //writeRawImagePixel(img, x, y, glm::vec4(0.0f + y * dist, 0.0f, 0.0f, 1.0f));
                }
            }
        }
    }
    else if (type == 1) {
        // modify by adding one white line
        for (int y = 0; y < img.height; y++) {
            for (int x = 0; x < img.width; x++) {
                if (x == y) {
                    writeRawImagePixel(img, x, y, Colors::White);
                }
            }
        }
    }
}

unsigned char floatToUnsignedChar(float value) {
    return static_cast<unsigned char>(value * 255.0f);
}

void Util::writeRawImagePixel(GPUImage& img, int x, int y, glm::vec4 color)
{
    unsigned char* data = (unsigned char*)img.imagedata;
    // add lines
    data += y * img.subResourceLayout.rowPitch;
    // add pixels
    data += x * 4;
    unsigned char r = floatToUnsignedChar(color.r);
    unsigned char g = floatToUnsignedChar(color.g);
    unsigned char b = floatToUnsignedChar(color.b);
    unsigned char a = floatToUnsignedChar(color.a);
    // no color swizzle
    //*data = r;
    //*(data + 1) = g;
    //*(data + 2) = b;
    //*(data + 3) = a;
    // color swizzle
    *data = b;
    *(data + 1) = g;
    *(data + 2) = r;
    *(data + 3) = a;
}

void Util::writePPM(std::string filename, const char* imagedata, uint64_t width, uint64_t height, uint64_t rowPitch, bool colorSwizzle)
{
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    // ppm header
    file << "P6\n" << width << "\n" << height << "\n" << 255 << "\n";

    // ppm binary pixel data
    for (int32_t y = 0; y < height; y++) {
        unsigned int* row = (unsigned int*)imagedata;
        for (int32_t x = 0; x < width; x++) {
            if (colorSwizzle) {
                file.write((char*)row + 2, 1);
                file.write((char*)row + 1, 1);
                file.write((char*)row, 1);
            }
            else {
                file.write((char*)row, 3);
            }
            row++;
        }
        imagedata += rowPitch;
    }
    file.close();

    //Log("written image dump file (PPM format): " << engine->files.absoluteFilePath(filename).c_str() << endl);
    Log("written image dump file (PPM format): " << filename.c_str() << endl);
}

void Util::writeHeightmapRaw(std::vector<glm::vec3>& points)
{
    stringstream name;
    name << "out_heightmap_" << setw(2) << setfill('0') << imageCounter++ << ".raw";
    //auto filename = engine.files.findFile(name.str(), FileCategory::TEXTURE, false, true); // we rather use current directory, maybe reconsider later
    auto filename = name.str();
    if (!engine->files.checkFileForWrite(filename)) {
        Log("Could not write image dump file: " << filename << endl);
        return;
    }
    // check that we have points that form a square
    double squareRoot = sqrt(points.size());
    int roundedSquareRoot = static_cast<int>(round(squareRoot));
    if (roundedSquareRoot * roundedSquareRoot != points.size()) {
        Error("Heightmap: points do not form a square");
    }
    //Log("Heightmap: write " << points.size() << " points" << endl);
    // the height values in the heightmap are in absolute world coordinates already -  we don't need to adapt them
    // Open a file in binary mode
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file) {
        return;
    }

    // Write the heightmap data to the file
    for (const glm::vec3& point : points) {
        float height = point.y;
        file.write(reinterpret_cast<const char*>(&height), sizeof(float));
        //Log(" height: " << height << endl);
    }

    // Close the file
    file.close();

    Log("written 32-bit float RAW heightmap file with ( " << roundedSquareRoot << " x " << roundedSquareRoot << " ) points: " << engine->files.absoluteFilePath(filename).c_str() << endl);
}

void Util::drawBoxFromAxes(std::vector<LineDef>& boxes, vec3* axes)
{
    LineDef l;
    l.color = Colors::LightSteelBlue;
    // lower rect:
    l.start = axes[0];
    l.end = axes[1];
    boxes.push_back(l);
}

#include <vulkan/vulkan.h>
#include <stdexcept>

uint32_t Util::getBytesPerPixel(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8_USCALED:
    case VK_FORMAT_R8_SSCALED:
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8_SRGB:
        return 1;

    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8_USCALED:
    case VK_FORMAT_R8G8_SSCALED:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8_SRGB:
        return 2;

    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_R8G8B8_USCALED:
    case VK_FORMAT_R8G8B8_SSCALED:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8_SRGB:
        return 3;

    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_B8G8R8_SNORM:
    case VK_FORMAT_B8G8R8_USCALED:
    case VK_FORMAT_B8G8R8_SSCALED:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8_SRGB:
        return 3;

    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_R8G8B8A8_USCALED:
    case VK_FORMAT_R8G8B8A8_SSCALED:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_USCALED:
    case VK_FORMAT_B8G8R8A8_SSCALED:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_B8G8R8A8_SRGB:
        return 4;

    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        return 4;

    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16_USCALED:
    case VK_FORMAT_R16_SSCALED:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16_SFLOAT:
        return 2;

    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16_USCALED:
    case VK_FORMAT_R16G16_SSCALED:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16_SFLOAT:
        return 4;

    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16_USCALED:
    case VK_FORMAT_R16G16B16_SSCALED:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16_SFLOAT:
        return 6;

    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_R16G16B16A16_USCALED:
    case VK_FORMAT_R16G16B16A16_SSCALED:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 8;

    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32_SFLOAT:
        return 4;

    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32_SFLOAT:
        return 8;

    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32_SFLOAT:
        return 12;

    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 16;

    case VK_FORMAT_R64_UINT:
    case VK_FORMAT_R64_SINT:
    case VK_FORMAT_R64_SFLOAT:
        return 8;

    case VK_FORMAT_R64G64_UINT:
    case VK_FORMAT_R64G64_SINT:
    case VK_FORMAT_R64G64_SFLOAT:
        return 16;

    case VK_FORMAT_R64G64B64_UINT:
    case VK_FORMAT_R64G64B64_SINT:
    case VK_FORMAT_R64G64B64_SFLOAT:
        return 24;

    case VK_FORMAT_R64G64B64A64_UINT:
    case VK_FORMAT_R64G64B64A64_SINT:
    case VK_FORMAT_R64G64B64A64_SFLOAT:
        return 32;

    // compressed formats return block size, not byte size
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return 16;

    default:
        throw std::runtime_error("Unsupported format");
    }
}

bool Util::isCompressedFormat(VkFormat format) {
    switch (format) {
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return true;
    default:
        return false;
    }
}

glm::vec3 Util::hsv2rgb(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1 - std::fabs(fmod(h * 6.0f, 2) - 1));
    float m = v - c;
    float r, g, b;
    if (h < 1.0f / 6.0f) { r = c; g = x; b = 0; }
    else if (h < 2.0f / 6.0f) { r = x; g = c; b = 0; }
    else if (h < 3.0f / 6.0f) { r = 0; g = c; b = x; }
    else if (h < 4.0f / 6.0f) { r = 0; g = x; b = c; }
    else if (h < 5.0f / 6.0f) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }
    return glm::vec3(r + m, g + m, b + m);
}

std::vector<glm::vec4> Util::generateColorPalette256()
{
    std::vector<glm::vec4> palette;
    palette.reserve(256);
    for (int i = 0; i < 256; ++i) {
        // Use bit-reversed index for hue
        float h = bit_reverse8(i) / 256.0f;
        float s = 0.75f;
        float v = 0.95f;
        glm::vec3 rgb = hsv2rgb(h, s, v);
        palette.emplace_back(rgb, 1.0f);
    }
    return palette;
}
