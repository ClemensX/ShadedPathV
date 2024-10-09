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

// handle object movement: cameras and worldObjects
class Path
{
public:
	// we need the terrain, the world and some other details to be able to calc path movement
	void init(World* world, WorldObject* terrain, UltimateHeightmapInfo* hinfo);
    void updateCameraPosition(Camera* camera, Movement& mv);
private:
	World* world = nullptr;
	WorldObject* terrain = nullptr;
	UltimateHeightmapInfo* hinfo = nullptr;
};

// handle world: terrain, heights, world size and grid
class World
{
public:
	// create Line Grid in x (width) and z (depth) direction
	void createGridXZ(Grid& grid);
    // create horizontal grid with lines. lineGap is the distance between lines, verticalAdjust is the height of the grid.
    // the outermost lines are always at the world border, while inner ones are placed at lineGap distance beginning at world center (0x 0z).
	Grid* createWorldGrid(float lineGap, float verticalAdjust = 0.0f);
	void setWorldSize(float x, float y, float z) {
		sizex = x; sizey = y; sizez = z;
		if (x != z) Error("World size needs to be square in x and z");
        minxz = -sizex / 2.0f;
        maxxz = sizex / 2.0f;
	};
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
	void setHeightmap(TextureID heightmap);
    float getHeightmapValueWC(float x, float z); // TODO decprecated, get heightmap value in world coords from WC heightmap

    // ultimate heightmap: perfect heightmap directly from terrain data

    // prepare ultimate heightmap calculation from terrain object
	// this MUST have been called before any call to getHeightmapValue()
	void prepareUltimateHeightmap(WorldObject* terrain);

    // any new terrain object should be checked at least once with this method.
	// the call can then be omitted for productive use
	void checkTerrainSuitableForHeightmap(WorldObject* terrain);

    // get heightmap value in world coords from ultimate heightmap
	// same precision as terrain data. Constant run time.
	float getHeightmapValue(float x, float z);

    // check if point is inside triangle, use with care: a point on or close to border line may erroneously return false
	bool isPointInTriangle(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);
	Path paths;
	UltimateHeightmapInfo ultHeightInfo;
private:
	glm::vec3 calculateBarycentricCoordinates(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);
	float interpolateY2(const glm::vec3& p, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
	float getHeightmapValue(UltimateHeightmapInfo& info, float x, float z);
	// world size in absolute units around origin, e.g. x is from -x to x
	float sizex = 0.0f, sizey = 0.0f, sizez = 0.0f;
    float minxz = 0.0f, maxxz = 0.0f; // for easy coord range checking
	Grid grid;
    TextureID heightmap = nullptr;
    float textureScaleFactor = 1.0f; // heightmap may be more or less detailed than world size
	size_t calcGridIndex(UltimateHeightmapInfo& info, float f);
    // get index into vertices array for given x and z. returning the index of the vertex with lowest x and z values.
    // the other three vertices of the triangle are then at index +1, +2 and +3
	size_t getSquareIndex(UltimateHeightmapInfo& info, int x, int z);
	size_t getTriangleIndex(UltimateHeightmapInfo& info, float x, float z);
};

