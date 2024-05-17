#include "mainheader.h"

using namespace std;

void GlobalUpdate::doGlobalShaderUpdates()
{
    if (!updateSetsCreated) {
		ctreateUpdateSets();
	}
    // this is the only place with pop():
    optional<GlobalUpdateElement*> opt_el = engine.getShaderUpdateQueue().pop();
    if (!opt_el) {
        return;
    }
}

void GlobalUpdate::ctreateUpdateSets()
{
	for (auto& shader : shaders) {
        shader->createUpdateSet(setA);
        shader->createUpdateSet(setB);
        shader->signalGlobalUpdateRunning(true); // TEST
    }
    updateSetsCreated = true;
}
