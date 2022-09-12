#pragma once
struct Grid {
	glm::vec4 center;
	float width; // total width (along x axis) of grid in units
	float depth; // total depth (along z axis) in units
	int widthCells; // number of cells to separate along x axis
	int depthCells; // number of cells to separate along z axis
	// store grid line endpoints in world coords, each pair of endpoints denotes one grid line to draw
	//vector<XMFLOAT4> zLineEndpoints; // parallel to z axis
	//vector<XMFLOAT4> xLineEndpoints; // parallel to x axis
	// lines mode:
	std::vector<LineDef> lines;
	// triangle mode with vertices and indexes:
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> tex;  //texture coords, stretch texture over entire area
	std::vector<UINT> indexes;
};

class World
{
public:
	// create Grid in x (width) and z (depth) direction, linesmode means create simple long lines, otherwise create vertex/index vectors
	void createGridXZ(Grid& grid, bool linesmode = true);
	Grid* createWorldGrid(float lineGap, float verticalAdjust = 0.0f);
	void setWorldSize(float x, float y, float z) { sizex = x; sizey = y; sizez = z; };
private:
	// world size in absolute units around origin, e.g. x is from -x to x
	float sizex = 0.0f, sizey = 0.0f, sizez = 0.0f;
	Grid grid;
};

