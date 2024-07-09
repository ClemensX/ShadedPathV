#include "mainheader.h"

using namespace glm;

void World::createGridXZ(Grid& grid, bool linesmode) {
	int zLineCount = grid.depthCells + 1;
	int xLineCount = grid.widthCells + 1;

	float halfWidth = grid.width / 2.0f;
	float halfDepth = grid.depth / 2.0f;

	float xstart = grid.center.x - halfWidth;
	float xend = grid.center.x + halfWidth;
	float xdiff = grid.width / grid.widthCells;
	float zstart = grid.center.z - halfDepth;
	float zend = grid.center.z + halfDepth;
	float zdiff = grid.depth / grid.depthCells;

	float x, z;
	if (linesmode == true) {
		LineDef line;
		line.color = Colors::Red;
		for (int xcount = 0; xcount < xLineCount; xcount++) {
			x = xstart + xcount * xdiff;
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
	}
	else {
		float du = 1.0f / (grid.widthCells);
		float dv = 1.0f / (grid.depthCells);
		for (int xcount = 0; xcount < xLineCount; xcount++) {
			x = xstart + xcount * xdiff;
			for (int zcount = 0; zcount < zLineCount; zcount++) {
				z = zstart + zcount * zdiff;
				glm::vec3 v(x, grid.center.y, z);
				grid.vertices.push_back(v);
				glm::vec2 t(du * xcount, dv * zcount);
				grid.tex.push_back(t);
			}
		}
	}
}

Grid* World::createWorldGrid(float lineGap, float verticalAdjust) {
	grid.center = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	grid.depth = sizez;
	grid.width = sizex;
	grid.depthCells = (int)(grid.depth / lineGap);
	grid.widthCells = (int)(grid.width / lineGap);
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
	center.y = 0.0f;
	glm::vec3 worldCenter(0.0f, 0.0f, 0.0f);
	glm::vec3 moveVec = worldCenter - center;
	transform = glm::translate(transform, moveVec);

	terrain->mesh->baseTransform = transform;
}
