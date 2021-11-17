#pragma once

// forward declarations
class ShadedPathEngine;

// global resources that are not changed in rendering threads.
// shader code, meshes, etc.
// all objects here are specific to shaders
// shader independent global objects like framebuffer, swap chain, render passes are in ShadedPathEngine
class GlobalRendering
{
private:
	// we need direct access to engine instance
	ShadedPathEngine& engine;

public:
	GlobalRendering(ShadedPathEngine& s) : engine(s) {
		Log("GlobalRendering c'tor\n");
	};
	~GlobalRendering() {
		Log("GlobalRendering destructor\n");
	};
	// detroy global resources, should only be called from engine dtor
	void destroy();
	Files files;
};

