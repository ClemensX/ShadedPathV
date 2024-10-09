#include "mainheader.h"

using namespace glm;

// Path

void Path::init(World* world, WorldObject* terrain, UltimateHeightmapInfo* hinfo)
{
	this->world = world;
	this->terrain = terrain;
	this->hinfo = hinfo;
}
