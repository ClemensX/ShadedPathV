#include "pch.h"
#include "Util.h"

using namespace std;

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
    pfnDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(engine->global.vkInstance, "vkSetDebugUtilsObjectNameEXT");
}

std::string Util::createDebugName(const char* name, ThreadResources& res) {
    return std::string(name) + " " + std::to_string(res.frameIndex);
}

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
        pfnDebugUtilsObjectNameEXT(engine->global.device, &nameInfo);
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

Spatial2D::Spatial2D(int N2plus1)
{
    // make sure we have a side length of form 2 * n + 1:
    double s = log2((double)(N2plus1 - 1));
    int n = (int)s;
    int n2 = 2 << (n-1);
    if (n2 != (N2plus1 - 1)) {
        throw std::out_of_range("Spatial2D needs side length initializer of the form (2 ^ N) + 1");
    }
    sidePoints = N2plus1;
    h = new float [N2plus1 * N2plus1];
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
