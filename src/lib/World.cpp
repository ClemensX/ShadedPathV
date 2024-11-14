#include "mainheader.h"

using namespace glm;

void World::createGridXZ(Grid& grid) {
	int zLineCount = grid.depthCells + 1;
	int xLineCount = grid.widthCells + 1;
    bool borderMode = false; // we draw border rectangle if border does not fall exactly on grid line

    // first draw grid with requested line gaps around center, stay inside world borders
	int gridWidthFittingHalf = grid.widthCells * grid.lineGap / 2;
	int gridDepthFittingHalf = grid.depthCells * grid.lineGap / 2;
    if (gridWidthFittingHalf * 2 < grid.width) {
        borderMode = true;
    }

	float xstart = grid.center.x - gridWidthFittingHalf;
    float xend = grid.center.x + gridWidthFittingHalf;
	float xdiff = grid.lineGap;
	float zstart = grid.center.z - gridDepthFittingHalf;
    float zend = grid.center.z + gridDepthFittingHalf;
	float zdiff = grid.lineGap;

	float x, z;
	LineDef line;
	line.color = Colors::Red;
	for (int xcount = 0; xcount < xLineCount; xcount++) {
		x = xstart + xcount * xdiff;
		//Log("world grid x: " << x << std::endl);
		vec3 p1(x, grid.center.y, zstart);
		vec3 p2(x, grid.center.y, zend);
		line.start = p1;
		line.end = p2;
		grid.lines.push_back(line);
		//grid.zLineEndpoints.push_back(p1);
		//grid.zLineEndpoints.push_back(p2);
	}
	for (int zcount = 0; zcount < zLineCount; zcount++) {
		z = zstart + zcount * zdiff;
		vec3 p1(xstart, grid.center.y, z);
		vec3 p2(xend, grid.center.y, z);
		line.start = p1;
		line.end = p2;
		grid.lines.push_back(line);
		//grid.xLineEndpoints.push_back(p1);
		//grid.xLineEndpoints.push_back(p2);
	}

    // now draw border rectangle if needed
	if (borderMode) {
		// draw border rectangle
		line.color = Colors::Silver;
        xstart = grid.center.x - grid.width / 2;
		xend = grid.center.x + grid.width / 2;
        zstart = grid.center.z - grid.depth / 2;
        zend = grid.center.z + grid.depth / 2;
		vec3 p1(xstart, grid.center.y, zstart);
		vec3 p2(xend, grid.center.y, zstart);
		line.start = p1;
		line.end = p2;
		grid.lines.push_back(line);
		p1 = p2;
		p2 = vec3(xend, grid.center.y, zend);
		line.start = p1;
		line.end = p2;
		grid.lines.push_back(line);
		p1 = p2;
		p2 = vec3(xstart, grid.center.y, zend);
		line.start = p1;
		line.end = p2;
		grid.lines.push_back(line);
		p1 = p2;
		p2 = vec3(xstart, grid.center.y, zstart);
		line.start = p1;
		line.end = p2;
		grid.lines.push_back(line);
	}
}

Grid* World::createWorldGrid(float lineGap, float verticalAdjust) {
	grid.center = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	grid.depth = sizez;
	grid.width = sizex;
	grid.depthCells = (int)(grid.depth / lineGap);
	grid.widthCells = (int)(grid.width / lineGap);
	grid.lineGap = lineGap;
	//createGridXZ(grid);
	float low = 0.0f + verticalAdjust;   // -sizey / 2.0f;
	float high = sizey + verticalAdjust; // / 2.0f;
	float step = lineGap;
	//for (float y = low; y <= high; y += step) {
	//	grid.center.y = y;
	//	//		createGridXZ(grid);
	//}
	//grid.center.y = 0;
	//createGridXZ(grid);
	grid.center.y = low;
	createGridXZ(grid);
	grid.center.y = high;
	createGridXZ(grid);
    // connect corners:
	LineDef line;
	line.color = Colors::Silver;
	float xstart = grid.center.x - grid.width / 2;
	float xend = grid.center.x + grid.width / 2;
	float zstart = grid.center.z - grid.depth / 2;
	float zend = grid.center.z + grid.depth / 2;
	vec3 p1(xstart, low, zstart);
	vec3 p2(xstart, high, zstart);
	line.start = p1;
	line.end = p2;
	grid.lines.push_back(line);
	p1 = vec3(xend, low, zstart);
	p2 = vec3(xend, high, zstart);
	line.start = p1;
	line.end = p2;
	grid.lines.push_back(line);
	p1 = vec3(xend, low, zend);
	p2 = vec3(xend, high, zend);
	line.start = p1;
	line.end = p2;
	grid.lines.push_back(line);
	p1 = vec3(xstart, low, zend);
	p2 = vec3(xstart, high, zend);
	line.start = p1;
	line.end = p2;
	grid.lines.push_back(line);

	return &grid;
}

vec3 World::getRandomPos() {
	return getRandomPos(0.0f);
}

vec3 World::getRandomPos(float minHeight) {
	float x = MathHelper::RandF(-sizex / 2.0f, sizex / 2.0f);
	float z = MathHelper::RandF(-sizez / 2.0f, sizez / 2.0f);
	float y = MathHelper::RandF(minHeight, sizey);
	return vec3(x, y, z);
}

void World::transformToWorld(WorldObject* terrain)
{
	if (sizex == 0.0f || sizey == 0.0f || sizez == 0.0f) {
        Error("World::transformToWorld: world size not set");
    }
	// compare terrain->mesh->baseTransform to identity matrix
	glm::mat4 identity(1.0f);
	if (terrain->mesh->baseTransform != identity) {
        Error("World::transformToWorld: terrain baseTransform not identity matrix");
    }

	// current terrain size:
	BoundingBox box;
	terrain->getBoundingBox(box);
	Log(" old terrain size: " << box.max.x - box.min.x << " " << box.max.y - box.min.y << " " << box.max.z - box.min.z << std::endl);
	float factor = sizex / (box.max.x - box.min.x);
	int factorRounded = (int)round(factor);
	if (abs((float)factorRounded - factor) > 0.1f) {
        Error("World::transformToWorld: terrain size not a multiple of world size");
    }
	// scale terrain to world size:
	glm::vec3 scaleVec(factor);
	glm::mat4 transform(1.0);
	transform = glm::scale(transform, scaleVec);

	// move terrain to world center:
	glm::vec3 center = (box.min + box.max) / 2.0f;
	center.y = 0;// -1000.0f;
	glm::vec3 worldCenter(0.0f, 0.0f, 0.0f);
	glm::vec3 moveVec = worldCenter - center;
	transform = glm::translate(transform, moveVec);
	terrain->mesh->baseTransform = transform;
}

void World::setHeightmap(TextureID heightmap)
{
	this->heightmap = heightmap;
	// heightmap size calculations:
	textureScaleFactor = sizex / (heightmap->vulkanTexture.width);
    Log("World heightmap has value every " << textureScaleFactor << " m" << std::endl);
}

float World::getHeightmapValueWC(float xp, float zp)
{
	//static size_t minIndex = heightmap->float_buffer.size() + 1000;
	//static size_t maxIndex = 0;
	// check that we are within world borders:
    if (xp < minxz || xp > maxxz || zp < minxz || zp > maxxz) {
        Error("World::getHeightmapValueWC: coordinates out of world borders");
    }

    // move world coords to positive range:
    float halfWorldSize = sizex / 2.0f;
	float x = xp + halfWorldSize;
	float z = zp + halfWorldSize;

    // scale world coords to texture coords:
    x /= textureScaleFactor;
    z /= textureScaleFactor;

    size_t index = (size_t)(z * heightmap->vulkanTexture.width + x);
	if (index < 0 || index >= heightmap->float_buffer.size()) {
		Error("World::getHeightmapValueWC: index out of range");
	}
    //if (index < minIndex) {
    //    minIndex = index;
    //    //Log("minIndex: " << minIndex << std::endl);
    //}
    //if (index > maxIndex) {
    //    maxIndex = index;
    //    //Log("maxIndex: " << maxIndex << std::endl);
    //}
	// convert world coords to texture coords:
	if (heightmap->hasFlag(TextureFlags::ORIENTATION_RAW_START_WITH_XMAX_ZMAX)) {
        size_t i = heightmap->float_buffer.size() - index - 1;
		return heightmap->float_buffer[i];
	} else {
		Error("World::getHeightmapValueWC: heightmap orientation not implemented");
	}
	return 0.0f;
}

// ultimate heightmap value getter, returns heightmap value at given world coordinates

float World::getHeightmapValue(float xp, float zp)
{
	// check that we are within world borders:
	if (xp < minxz || xp > maxxz || zp < minxz || zp > maxxz) {
		Error("World::getHeightmapValueWC: coordinates out of world borders");
	}

	// move world coords to positive range:
	float halfWorldSize = sizex / 2.0f;
	float x = xp + halfWorldSize;
	float z = zp + halfWorldSize;

    return getHeightmapValue(ultHeightInfo, x, z);
}

// Function to calculate the barycentric coordinates
glm::vec3 World::calculateBarycentricCoordinates(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
	glm::vec3 v0 = b - a;
	glm::vec3 v1 = c - a;
	glm::vec3 v2 = p - a;
	float d00 = glm::dot(v0, v0);
	float d01 = glm::dot(v0, v1);
	float d11 = glm::dot(v1, v1);
	float d20 = glm::dot(v2, v0);
	float d21 = glm::dot(v2, v1);
	float denom = d00 * d11 - d01 * d01;
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;
	return glm::vec3(u, v, w);
}

size_t World::calcGridIndex(UltimateHeightmapInfo& info, float f)
{
	size_t ret;
    size_t slot = (size_t)(f / info.calcDist);
	if (slot <= 0) return 0;
    if (slot >= info.gridIndex.size()) return info.gridIndex.size()-2; // border case
	if (info.gridIndex[slot] <= f) ret = slot;
	else if (info.gridIndex[slot-1] <= f) ret = slot-1;
	if (ret < info.gridIndex.size() - 1) {
		assert(info.gridIndex[ret] <= f && info.gridIndex[ret + 1] > f);
	}
	return ret;
}

/*
 we assume this order in indices vector:
 first triangle: p0,p1,p2 
 2nd             p3,p4,p5
        P4
P1      P2 
----------
|\       |
| \      |
|  \     |
|   \    |
|    \   |
|     \  |
|      \ |
|       \|
----------
P0      P5
P3

*/
size_t World::getSquareIndex(UltimateHeightmapInfo& info, int x, int z)
{
    WorldObject* terrain = info.terrain;
	if (false) {
		vec3& v0 = terrain->mesh->vertices[terrain->mesh->indices[0]].pos;
		vec3& v1 = terrain->mesh->vertices[terrain->mesh->indices[1]].pos;
		vec3& v2 = terrain->mesh->vertices[terrain->mesh->indices[2]].pos;
		vec3& v3 = terrain->mesh->vertices[terrain->mesh->indices[3]].pos;
		vec3& v4 = terrain->mesh->vertices[terrain->mesh->indices[4]].pos;
		vec3& v5 = terrain->mesh->vertices[terrain->mesh->indices[5]].pos;
		// assert we have memory layout of z changing first, then x
		assert(v0.x == v1.x && v1.z > v0.z);
	}

    size_t index = z * info.squaresPerLine + x;
	index *= 6;
	if (false) {
		vec3& v0 = terrain->mesh->vertices[terrain->mesh->indices[index]].pos;
		vec3& v1 = terrain->mesh->vertices[terrain->mesh->indices[index + 1]].pos;
		vec3& v2 = terrain->mesh->vertices[terrain->mesh->indices[index + 2]].pos;
		vec3& v3 = terrain->mesh->vertices[terrain->mesh->indices[index + 3]].pos;
		vec3& v4 = terrain->mesh->vertices[terrain->mesh->indices[index + 4]].pos;
		vec3& v5 = terrain->mesh->vertices[terrain->mesh->indices[index + 5]].pos;
        Log("v0: " << v0.x << " " << v0.y << " " << v0.z << std::endl);
	}
	return index;
}

size_t World::getTriangleIndex(UltimateHeightmapInfo& info, float x, float z)
{
	WorldObject* terrain = info.terrain;
	size_t xI = calcGridIndex(ultHeightInfo, x);
	size_t zI = calcGridIndex(ultHeightInfo, z);
	size_t idx = getSquareIndex(ultHeightInfo, xI, zI);

	vec3& v0 = terrain->mesh->vertices[terrain->mesh->indices[idx]].pos;
	// if slope > 1, we are in the upper triangle, otherwise in the lower one
    float slope = (v0.z - z) / (v0.x - x);
    if (slope >= 1.0f) {
        return idx;
	} else {
		return idx + 3;
	}
}

float World::getHeightmapValue(UltimateHeightmapInfo& info, float x, float z)
{
	WorldObject* terrain = info.terrain;
	size_t idx = getTriangleIndex(info, x, z);
    vec3& v0 = terrain->mesh->vertices[terrain->mesh->indices[idx]].pos;
    vec3& v1 = terrain->mesh->vertices[terrain->mesh->indices[idx + 1]].pos;
    vec3& v2 = terrain->mesh->vertices[terrain->mesh->indices[idx + 2]].pos;
    vec3 p(x, 0.0f, z);
    float h = interpolateY2(p, v0, v1, v2);
	//p.y = h;
	//bool isInside = isPointInTriangle(p, v0, v1, v2);
	//   if (!isInside) {
	//	Log("is not Inside! " << std::endl);
	//	isInside = isPointInTriangle(p, v0, v1, v2);
	//}
	return h;
}

// Function to interpolate the y value
float World::interpolateY2(const glm::vec3& p, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
	float a0 = p.x;
	float a1 = p.z;
	float a2 = v0.x;
	float a3 = v0.z;
	float a4 = v1.x;
	float a5 = v1.z;
	float a6 = v2.x;
	float a7 = v2.z;
	float a8 = v0.y;
	float a9 = v1.y;
	float a10 = v2.y;

	float x = a0 - a2;
    float y = a1 - a3;
    float e1x = a4 - a2;
    float e1y = a5 - a3;
    float e2x = a6 - a2;
    float e2y = a7 - a3;
    float id = 1.0f / (e1x * e2y - e1y * e2x);
    float x0 = x * e2y * id + y * -e2x * id;
    float y0 = x * -e1y * id + y * e1x * id;
    return a8 * (1 - x0 - y0) + a9 * x0 + a10 * y0;
}

// Function to check if a point is within a triangle
bool World::isPointInTriangle(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
	glm::vec3 baryCoords = calculateBarycentricCoordinates(p, a, b, c);
	return (baryCoords.x >= 0.0f && baryCoords.y >= 0.0f && baryCoords.z >= 0.0f &&
		baryCoords.x <= 1.0f && baryCoords.y <= 1.0f && baryCoords.z <= 1.0f);
}

void World::checkTerrainSuitableForHeightmap(WorldObject* terrain)
{
	size_t indexCount = terrain->mesh->indices.size();
	assert(indexCount % 3 == 0);
	UltimateHeightmapInfo ultHeightInfo;
	ultHeightInfo.numTriangles = indexCount / 3;
	ultHeightInfo.numSquares = ultHeightInfo.numTriangles / 2;
	ultHeightInfo.squaresPerLine = sqrt(ultHeightInfo.numSquares);
	assert((ultHeightInfo.squaresPerLine * ultHeightInfo.squaresPerLine) * 2 * 3 == indexCount);

    // iterate over all triangles in mesh data to get all x and z coords used
	// they have to be nearly equidistant (on a grid)
	for (size_t i = 0; i < indexCount; i += 3) {
		glm::vec3 v0 = terrain->mesh->vertices[terrain->mesh->indices[i]].pos;
		glm::vec3 v1 = terrain->mesh->vertices[terrain->mesh->indices[i + 1]].pos;
		glm::vec3 v2 = terrain->mesh->vertices[terrain->mesh->indices[i + 2]].pos;
		ultHeightInfo.sortedSetX.insert(v0.x);
		ultHeightInfo.sortedSetX.insert(v1.x);
		ultHeightInfo.sortedSetX.insert(v2.x);
		ultHeightInfo.sortedSetZ.insert(v0.z);
		ultHeightInfo.sortedSetZ.insert(v1.z);
		ultHeightInfo.sortedSetZ.insert(v2.z);
	}
	if (ultHeightInfo.sortedSetX.size() != ultHeightInfo.squaresPerLine + 1) {
		Error("World::ultimateHeightmapCalculation: terrain data is not on grid");
	}
	if (ultHeightInfo.sortedSetZ.size() != ultHeightInfo.squaresPerLine + 1) {
		Error("World::ultimateHeightmapCalculation: terrain data is not on grid");
	}
	auto it = ultHeightInfo.sortedSetX.begin();
	float el0 = *it;
	float el1 = *++it;
	float fixedDiff = el1 - el0;
	float last = std::numeric_limits<double>::quiet_NaN();
	for (auto& value : ultHeightInfo.sortedSetX) {
		if (!std::isnan(last)) {
			float diff = value - last;
			if (diff - fixedDiff > 0.001f) {
				Log("diff: " << diff << std::endl);
				Error("terrain unsuitable for heightmap creation: not equidistant coords");
			}
		}
		last = value;
	}
	// ensure same grid distances in X and Z
	std::set<float>::iterator itx, itz;
	for (itx = ultHeightInfo.sortedSetX.begin(), itz = ultHeightInfo.sortedSetZ.begin();
		itx != ultHeightInfo.sortedSetX.end() && itz != ultHeightInfo.sortedSetZ.end(); ++itx, ++itz) {
		if (!(*itx == *itz)) Error("World::ultimateHeightmapCalculation: terrain data grid distances not the same in X and Z");
	}
	float lastEl = *ultHeightInfo.sortedSetX.rbegin();
	//Log("last el " << lastEl << " dist calc: " << (lastEl / ultHeightInfo.sortedSetX.size()) << std::endl);
	ultHeightInfo.maxXZ = lastEl;
	ultHeightInfo.calcDist = lastEl / ultHeightInfo.sortedSetX.size();
	ultHeightInfo.gridIndex.resize(ultHeightInfo.sortedSetX.size());
	size_t i = 0;
	for (auto& el : ultHeightInfo.sortedSetX) {
		ultHeightInfo.gridIndex[i] = el;
		i++;
	}
	ultHeightInfo.terrain = terrain;
}

void World::prepareUltimateHeightmap(WorldObject* terrain)
{
	size_t indexCount = terrain->mesh->indices.size();
	ultHeightInfo.numTriangles = indexCount / 3;
	ultHeightInfo.numSquares = ultHeightInfo.numTriangles / 2;
	ultHeightInfo.squaresPerLine = sqrt(ultHeightInfo.numSquares);
    size_t indicesPerLine = ultHeightInfo.squaresPerLine * 6;

    // iterate over one line of triangles in mesh data to prepare the sorted set of x and z coords
    for (size_t i = 0; i < indicesPerLine; i += 3) {
        glm::vec3 v0 = terrain->mesh->vertices[terrain->mesh->indices[i]].pos;
        glm::vec3 v1 = terrain->mesh->vertices[terrain->mesh->indices[i + 1]].pos;
        glm::vec3 v2 = terrain->mesh->vertices[terrain->mesh->indices[i + 2]].pos;
		ultHeightInfo.sortedSetX.insert(v0.x);
		ultHeightInfo.sortedSetX.insert(v1.x);
		ultHeightInfo.sortedSetX.insert(v2.x);
	}
	if (ultHeightInfo.sortedSetX.size() != ultHeightInfo.squaresPerLine + 1) {
		Error("World::ultimateHeightmapCalculation: terrain data is not on grid");
	}
    float lastEl = *ultHeightInfo.sortedSetX.rbegin();
    Log("last el " << lastEl << " dist calc: " << (lastEl / ultHeightInfo.sortedSetX.size()) << std::endl);
	ultHeightInfo.maxXZ = lastEl;
	ultHeightInfo.calcDist = lastEl / ultHeightInfo.sortedSetX.size();
    ultHeightInfo.gridIndex.resize(ultHeightInfo.sortedSetX.size());
    size_t i = 0;
    for (auto& el : ultHeightInfo.sortedSetX) {
        ultHeightInfo.gridIndex[i] = el;
        i++;
    }
	ultHeightInfo.terrain = terrain;

	// test border cases:
	bool testing = false;
	if (testing) {
		float h = getHeightmapValue(-200.0f, 512.0f);
		h = getHeightmapValue(-512.0f, 512.0f);
		h = getHeightmapValue(512.0f, -512.0f);
		h = getHeightmapValue(512.0f, 512.0f);
		h = getHeightmapValue(-512.0f, -512.0f);
		h = getHeightmapValue(-200.0f, 500.0f);

        float hwc = getHeightmapValueWC(-200.0f, 500.0f);
		Log("hwc: " << hwc << std::endl);
	}
}
