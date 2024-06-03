#pragma once

// this class implements a general scheme to update shader resources in the background
// there are always two sets of resources, A and B and this class is responsible for triggering updating one set while the other is in use

enum class GlobalUpdateDesignator { SET_A, SET_B };

// used in all the global update methods to signal upadte status
struct GlobalUpdateElement {
	std::atomic<bool> free = true;   // can be reserved
	std::atomic<bool> readyToRender = false; // update is ready to be rendered
	std::atomic<bool> active = false; // true: somebody uses this for rendering, false: should be applied
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
	// signal that global update is currently being prepared,
	// so single thread resources in application code should not be updated
	// during updatePerFrame() call
	virtual bool signalGlobalUpdateRunning(bool isRunning) = 0;
	virtual void updateGlobal(GlobalUpdateElement& currentSet) = 0;
};

// all methofds run in update thread unless otherwise noted
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

	// called every cycle from main engine
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
		//bool setAcanBeApplied = setA.readyToRender && !setA.active;
		//bool setBcanBeApplied = setB.readyToRender && !setB.active;
		//if (setAcanBeApplied && setBcanBeApplied) {
		//	Error("Both update sets are ready to render, this should not happen\n");
		//}
		//if (setAcanBeApplied && currentUpdateSet != &setA) return &setA;
		//if (setBcanBeApplied && currentUpdateSet != &setB) return &setB;
		return nullptr;
	}
	GlobalUpdateElement* getDispensableGlobalUpdateSet(GlobalUpdateElement* currentUpdateSet) {
		// if we are not currently rendering there is nothing to dispense
		if (currentUpdateSet == nullptr) return nullptr;
		GlobalUpdateElement* changedGlobalUpdateSet = getChangedGlobalUpdateSet(currentUpdateSet, -1);
		if (changedGlobalUpdateSet != nullptr) {
			// if we apply another update set, we can dispense the current one
			return currentUpdateSet;
		}
		return nullptr;
	}

	long getNextUpdateNumber()
	{
		long n = nextFreeUpdateNum++;
		return n;
	}

private:
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
};

