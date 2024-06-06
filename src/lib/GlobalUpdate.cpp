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
	signalGlobalUpdateRunning(true);
	//if (setA.usedByShaders) Log("setA used by shaders\n");
	//if (setB.usedByShaders) Log("setB used by shaders\n");
	GlobalUpdateElement* currentSet = nullptr;
	{
		std::unique_lock<std::mutex> lock(maintenanceMutex);
		if (!setA.usedByShaders) {
			setA.free = true;
		}
		if (!setB.usedByShaders) {
			setB.free = true;
		}
		if (!isInactiveSetAvailable()) {
			Log("WARNING: skipping global update - no slot available\n");
			signalGlobalUpdateRunning(false);
			return;
		}
		// we just ask every shader to update itself,
		// TODO maybe just call the shader that pushed()?
		currentSet = &getInactiveSet();
		currentSet->updateNumber = getNextUpdateNumber();
		currentSet->free = false;
		currentSet->readyToRender = false;
	}

	for (auto& shader : shaders) {
		shader->updateGlobal(*currentSet);
	}
	currentSet->readyToRender = true;
	signalGlobalUpdateRunning(false);
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

void GlobalUpdate::singleDrawingThreadMaintenance()
{
	setA.usedByShaders = false;
	setB.usedByShaders = false;
	for (auto& shader : shaders) {
		for (ThreadResources& res : engine.threadResources) {
			//Log("maintenance for drawing thread " << tr->frameIndex << endl);
			bool used = res.currentGlobalUpdateElement == &setA;
			if (used) {
				//Log("setA used by shader, prev usage: " << setA.usedByShaders << endl);
				setA.usedByShaders = true;
			}
			used = res.currentGlobalUpdateElement == &setB;
			if (used) {
				//Log("setB used by shader, prev usage: " << setB.usedByShaders << endl);
				setB.usedByShaders = true;
			}
		}
	}
	//long fn = engine.threadResources[0].frameNum;
	//Log("singleDrawingThreadMaintenance() frameNum: " << fn << endl);
	//Log("         setA.free: " << setA.free << " usedbyshader " << setA.usedByShaders << endl);
	//Log("         setB.free: " << setB.free << " usedbyshader " << setB.usedByShaders << endl);
}

void GlobalUpdate::signalGlobalUpdateRunning(bool isRunning)
{
	globalUpdateRunning = isRunning;
}

void GlobalUpdate::markGlobalUpdateSetAsUsed(GlobalUpdateElement* updateSet, ThreadResources& tr) {
	tr.currentGlobalUpdateElement = updateSet; // TODO: what if multiple shaders use different sets in same frame?
}
