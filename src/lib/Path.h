// handle object movement: cameras and worldObjects
class Path
{
public:
	// we need the terrain, the world and some other details to be able to calc path movement
	void init(World* world, WorldObject* terrain, UltimateHeightmapInfo* hinfo);
    void updateCameraPosition(CameraPositionerInterface* camera, Movement& mv, double deltaSeconds);
	bool isStandingStill(Movement& mv);
	glm::vec3 moveNoGradient(glm::vec3 start, const glm::vec3& direction, float speed, float deltaTime);
private:
	World* world = nullptr;
	WorldObject* terrain = nullptr;
	UltimateHeightmapInfo* hinfo = nullptr;
};
