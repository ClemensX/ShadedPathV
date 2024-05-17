#pragma once

// this class implements a general scheme to update shader resources in the background
// there are always two sets of resources, A and B and this class is responsible for triggering updating one set while the other is in use

enum class GlobalUpdateDesignator { SET_A, SET_B };

// all shaders have to subclass this for their update array
struct GlobalUpdateElement {
	//std::atomic<bool> free = true;   // can be reserved
	//bool inuse = false; // update process in progress
	//unsigned long num = 0; // count updates
	//size_t arrayIndex = 0; // we need to know array index into updateArray by having just a pointer to an element
	//GlobalResourceSet globalResourceSet;
	ShaderBase* shaderInstance = nullptr;
	GlobalUpdateDesignator updateDesignator;
};

// base class for shaders using global update
class GlobalUpdateBase {
public:
	// all update sets have to be creted in deactive state
	virtual void createUpdateSet(GlobalUpdateElement& el) = 0;
	// signal that global update is currently being prepared,
	// so single thread resources in application code should not be updated
	// during updatePerFrame() call
	virtual void signalGlobalUpdateRunning(bool isRunning) = 0;
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
};

