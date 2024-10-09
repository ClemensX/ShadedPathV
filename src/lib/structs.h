#pragma once

struct Grid {
	glm::vec4 center;
	float width; // total width (along x axis) of grid in units
	float depth; // total depth (along z axis) in units
	int widthCells; // number of cells to separate along x axis
	int depthCells; // number of cells to separate along z axis
	// store grid line endpoints in world coords, each pair of endpoints denotes one grid line to draw
	std::vector<LineDef> lines;
	float lineGap; // distance between lines
};

struct UltimateHeightmapInfo {
	size_t numTriangles;
	size_t numSquares;
	size_t squaresPerLine;
	// sorted set of x grid coords
	std::set<float> sortedSetX;
	// sorted set of z grid coords
	std::set<float> sortedSetZ;
	float maxXZ; // biggest x and z coord
	float calcDist; // calculated distance between grid lines
	std::vector<float> gridIndex; // used to calc the right index for given x or z float.
	WorldObject* terrain = nullptr;
};

