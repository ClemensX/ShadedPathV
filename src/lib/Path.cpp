#include "mainheader.h"

using namespace glm;
using namespace std;

// Path

void Path::init(World* world, WorldObject* terrain, UltimateHeightmapInfo* hinfo)
{
	this->world = world;
	this->terrain = terrain;
	this->hinfo = hinfo;
}

void Path::updateCameraPosition(CameraPositionerInterface* camera, Movement& mv, double deltaSeconds)
{
    auto cam = camera->getPosition();
	float terrainHeight = world->getHeightmapValue(cam.x, cam.z);
    float intendedHeight = terrainHeight + mv.camAboveGround;
    float heightAboveGround = cam.y - intendedHeight;
    Log("heightAboveGround: " << heightAboveGround << " terrain h " << terrainHeight << endl);
    if (abs(heightAboveGround) >= mv.fallHeight) {
        // we are falling
        mv.type = MovementType::Falling;
        // falling straight down for now, no other movement vectors
        float fallingDistance = mv.fallSpeedMS * deltaSeconds;
        // make sure we don't overshoot
        if (abs(heightAboveGround) > fallingDistance) {
            if (heightAboveGround > 0) {
                cam.y -= fallingDistance;
            } else {
                cam.y += fallingDistance;
            }
        } else {
            cam.y = intendedHeight;
        }
    } else {
        mv.type = MovementType::Walking;
    }
    camera->setPosition(cam);
}
