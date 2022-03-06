#include "pch.h"

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
	for (float y = low; y <= high; y += step) {
		grid.center.y = y;
		//		createGridXZ(grid);
	}
	//grid.center.y = 0;
	//createGridXZ(grid);
	grid.center.y = low;
	createGridXZ(grid);
	grid.center.y = high;
	createGridXZ(grid);
	return &grid;
}
