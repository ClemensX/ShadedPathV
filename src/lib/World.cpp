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
		Log("world grid x: " << x << std::endl);
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

float World::getHeightmapValue(float xp, float zp)
{
    // check that we are within world borders:
    if (xp < minxz || xp > maxxz || zp < minxz || zp > maxxz) {
        Error("World::getHeightmapValue: coordinates out of world borders");
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
		Error("World::getHeightmapValue: index out of range");
	}
	// convert world coords to texture coords:
	if (heightmap->hasFlag(TextureFlags::ORIENTATION_RAW_START_WITH_XMAX_ZMAX)) {
        size_t i = heightmap->float_buffer.size() - index - 1;
		return heightmap->float_buffer[i];
	} else {
		Error("World::getHeightmapValue: heightmap orientation not implemented");
	}
	return 0.0f;
}