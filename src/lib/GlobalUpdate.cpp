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
	if (!isInactiveSetAvailable()) {
		Log("WARNING: skipping global update - no slot available\n");
		return;
	}
	// we just ask every shader to update itself,
	// TODO maybe just call the shader that pushed()?
	GlobalUpdateElement& currentSet = getInactiveSet();
	currentSet.updateNumber = getNextUpdateNumber();
	currentSet.free = false;
	for (auto& shader : shaders) {
		if (shader->signalGlobalUpdateRunning(true)) {
			shader->updateGlobal(currentSet);
			shader->signalGlobalUpdateRunning(false);
		}
	}
	currentSet.readyToRender = true;
}

void GlobalUpdate::ctreateUpdateSets()
{
	for (auto& shader : shaders) {
        shader->createUpdateSet(setA);
        shader->createUpdateSet(setB);
    }
    updateSetsCreated = true;
}

GlobalUpdateElement& GlobalUpdate::getInactiveSet()
{
	if (setA.free) {
		return setA;
	} else if (setB.free) {
		return setB;
	}
	Error("Both update sets are in use, this should not happen\n");
	return setA; // keep compiler happy
}