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
    static float bug_dist = 0.0f, bug_time = 0.0f;
    auto cam = camera->getPosition();
	float terrainHeight = world->getHeightmapValue(cam.x, cam.z);
    float intendedHeight = terrainHeight + mv.camAboveGround;
    float heightAboveGround = cam.y - intendedHeight;
    //Log("heightAboveGround: " << heightAboveGround << " terrain h " << terrainHeight << endl);
    if (mv.type == MovementType::Falling && abs(heightAboveGround) >= mv.fallHeight) {
        // we are falling
        mv.type = MovementType::Falling;
        // falling straight down for now, no other movement vectors
        float fallingDistance = mv.fallSpeedMS * deltaSeconds;
        // make sure we don't overshoot
        if (abs(heightAboveGround) > fallingDistance) {
            if (heightAboveGround > 0) {
                cam.y -= fallingDistance;
            } else {
                // if for some reason we are under terrain we want to move up
                cam.y += fallingDistance;
            }
        } else {
            cam.y = intendedHeight;
        }
    } else {
        mv.type = MovementType::Walking;
        if (isStandingStill(mv)) {
            //Log("Standing still" << endl);
            return;
        }
        if (mv.forward_) {
            // move forward in the direction we are looking
            vec3 move = cam;
            cam = moveNoGradient(cam, mv.forward, mv.walkSpeedMS, deltaSeconds);//mv.forward * mv.speed * static_cast<float>(deltaSeconds);
            cam.y += mv.camAboveGround;
            bug_dist += sqrt(pow(cam.x - move.x, 2) + pow(cam.z - move.z, 2));
            bug_time += deltaSeconds;
            //Log("cam x/z: " << cam.x << " / " << cam.z << " total dist time: " << bug_dist << " " << bug_time << endl);
        }
        if (mv.backward_) {
            // move backward in the direction we are looking
            cam = moveNoGradient(cam, -mv.forward, mv.walkSpeedMS, deltaSeconds);
            cam.y += mv.camAboveGround;
        }
        if (mv.left_) {
            // move left in the direction we are looking
            cam = moveNoGradient(cam, -mv.right, mv.walkSpeedMS, deltaSeconds);
            cam.y += mv.camAboveGround;
        }
        if (mv.right_) {
            // move right in the direction we are looking
            cam = moveNoGradient(cam, mv.right, mv.walkSpeedMS, deltaSeconds);
            cam.y += mv.camAboveGround;
        }
        //Log("Walking" << endl);
    }
    camera->setPosition(cam);
}

bool Path::isStandingStill(Movement& mv)
{
    if (mv.forward_ || mv.backward_ || mv.left_ || mv.right_ || mv.up_ || mv.down_) {
        return false;
    }
    return true;
}

vec3 Path::moveNoGradient(vec3 start, const vec3& direction, float speed, float deltaTime) {
    glm::vec3 position = start;
    // Normalize the direction vector
    glm::vec3 normalizedDirection = glm::normalize(direction);

    // Calculate the movement vector
    glm::vec3 movement = normalizedDirection * speed * deltaTime;

    // Update the position
    position += movement;

    // Adjust for terrain height
    position.y = world->getHeightmapValue(position.x, position.z);
    return position;
}
