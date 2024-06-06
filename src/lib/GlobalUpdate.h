#pragma once

// this class implements a general scheme to update shader resources in the background
// there are always two sets of resources, A and B and this class is responsible for triggering updating one set while the other is in use
// update sets are only reserved for use by the global update thread, but then can only be put back to being unused
// if all shaders have finished using it. This is checked by singleDrawingThreadMaintenance() after each drame has been drawn.

class ThreadResources;
enum class GlobalUpdateDesignator { SET_A, SET_B };

// used in all the global update methods to signal upadte status
struct GlobalUpdateElement {
	std::atomic<bool> free = true;   // can be reserved
	std::atomic<bool> readyToRender = false; // update is ready to be rendered
	//std::atomic<bool> active = false; // true: somebody uses this for rendering, false: should be applied
	bool usedByShaders = false; // true: some shader is using this set, false: can be dispensed (filled during singleDrawingThreadMaintenance())
	long updateNumber = -1; // unique number for each update, always increasing
	//bool inuse = false; // update process in progress
	//unsigned long num = 0; // count updates
	//size_t arrayIndex = 0; // we need to know array index into updateArray by having just a pointer to an element
	//GlobalResourceSet globalResourceSet;
	ShaderBase* shaderInstance = nullptr;
	GlobalUpdateDesignator updateDesignator;
	static std::string to_string(GlobalUpdateDesignator des) {
		switch (des) {
		case GlobalUpdateDesignator::SET_A:
			return "SET_A";
		case GlobalUpdateDesignator::SET_B:
			return "SET_B";
		default:
			return "Unknown";
		}
	};
	std::string to_string() {
		return to_string(updateDesignator);
	}
};

// base class for shaders using global update
class GlobalUpdateBase {
public:
	// all update sets have to be creted in deactive state
	virtual void createUpdateSet(GlobalUpdateElement& el) = 0;
	virtual void updateGlobal(GlobalUpdateElement& currentSet) = 0;
	virtual void freeUpdateResources(GlobalUpdateElement* updateSet) = 0;

};

// all methods run in update thread unless otherwise noted
class GlobalUpdate
{
private:
	// we need direct access to engine instance
	ShadedPathEngine& engine;
	GlobalUpdateElement setA, setB;
	std::vector<GlobalUpdateBase*> shaders;
	bool updateSetsCreated = false;

public:
	GlobalUpdate(ShadedPathEngine& s) : engine(s) {
		Log("GlobalUpdate c'tor\n");
		setA.updateDesignator = GlobalUpdateDesignator::SET_A;
		setB.updateDesignator = GlobalUpdateDesignator::SET_B;
	};

	~GlobalUpdate() {
		Log("GlobalUpdate destructor\n");
	};

	// called continously in ShadedPathEngine::runUpdateThread
	void doGlobalShaderUpdates();
	// called once to let all shaders intitialize for updates.
	void ctreateUpdateSets();
	void registerShader(GlobalUpdateBase* shader) {
		shaders.push_back(shader);
	}

	// util methods for shaders

	// find the update set that should be applied, nullptr means none
	// the current drawing thread only needs to know if it need to switch to another set
	// called only from drawing threads
	GlobalUpdateElement* getChangedGlobalUpdateSet(GlobalUpdateElement* currentUpdateSet, long currentlyRenderingUpdateNumber) {
		if (shouldUpdateToSet(setA, currentUpdateSet, currentlyRenderingUpdateNumber))
			return &setA;
		if (shouldUpdateToSet(setB, currentUpdateSet, currentlyRenderingUpdateNumber))
			return &setB;
		return nullptr;
	}

	long getNextUpdateNumber()
	{
		long n = nextFreeUpdateNum++;
		return n;
	}

	void doSyncedDrawingThreadMaintenance() {
		{
			std::unique_lock<std::mutex> lock(maintenanceMutex);
			singleDrawingThreadMaintenance();
		}
	}

	bool isRunning() {
		return globalUpdateRunning;
	}

	void markGlobalUpdateSetAsUsed(GlobalUpdateElement* updateSet, ThreadResources& tr);

private:
	mutable std::mutex maintenanceMutex; // used for maintenance tasks that have to be run with no other drawing thread running
	void singleDrawingThreadMaintenance();


	bool shouldUpdateToSet(GlobalUpdateElement& set, GlobalUpdateElement* currentUpdateSet, long currentlyRenderingUpdateNumber) {
		// only readyToRender sets can be updated
		if (!set.readyToRender)	return false;
		// easy case: no current update set
		if (currentUpdateSet == nullptr) return true;
		// if we are currently rendering the set, we cannot update it
		if (currentUpdateSet == &set) return false;
		// if the update number is lower than the current rendering update number, we can update
		if (currentlyRenderingUpdateNumber < set.updateNumber) return true;
		return false;
	}
	GlobalUpdateElement& getInactiveSet();
	bool isInactiveSetAvailable() {
		return setA.free || setB.free;
	}
	std::atomic<long> nextFreeUpdateNum = 0;
	std::atomic<bool> globalUpdateRunning = false;
	// signal that global update is currently being prepared,
	// so single thread resources in application code should not be updated
	// during updatePerFrame() call
	void signalGlobalUpdateRunning(bool isRunning);

};

