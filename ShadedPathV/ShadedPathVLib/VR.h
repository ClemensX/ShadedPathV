#pragma once
class VR
{
public:
	VR(ShadedPathEngine& s) : engine(s) {
		Log("VR c'tor\n");
	};
	~VR();

	void init();
	// if false we run without VR
	bool enabled = false;
	void initAfterDeviceCreation();
	void initGLFW();
	void createPresentQueue(unsigned int value);


	bool shouldClose();

private:
	ShadedPathEngine& engine;

};


