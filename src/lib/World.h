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

class World
{
public:
	// create Line Grid in x (width) and z (depth) direction
	void createGridXZ(Grid& grid);
    // create horizontal grid with lines. lineGap is the distance between lines, verticalAdjust is the height of the grid.
    // the outermost lines are always at the world border, while inner ones are placed at lineGap distance beginning at world center (0x 0z).
	Grid* createWorldGrid(float lineGap, float verticalAdjust = 0.0f);
	void setWorldSize(float x, float y, float z) { sizex = x; sizey = y; sizez = z; if (x != z) Error("World size needs to be square in x and z"); };
	// randomly generate one location within the defined world coords
	glm::vec3 getRandomPos();
	// randomly generate one location within the defined world coords with a given minimum height
	glm::vec3 getRandomPos(float minHeight);
	// get world sizes (read-only)
	const glm::vec3 getWorldSize() {
		glm::vec3 s(sizex, sizey, sizez);
		return s;
	}
	// transform terrain object to match world size and position
	void transformToWorld(WorldObject* obj);
private:
	// world size in absolute units around origin, e.g. x is from -x to x
	float sizex = 0.0f, sizey = 0.0f, sizez = 0.0f;
	Grid grid;
};

