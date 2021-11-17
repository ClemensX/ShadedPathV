#pragma once

// forward declarations
class ShadedPathEngine;

class Presentation
{
public:
	Presentation(ShadedPathEngine& s) : engine(s) {
		Log("Presentation c'tor\n");
	};
	~Presentation() {
		Log("Presentation destructor\n");
	};
	bool shouldClose();

	// if false we run on headless mode
	bool enabled = false;

private:
	ShadedPathEngine& engine;
};

