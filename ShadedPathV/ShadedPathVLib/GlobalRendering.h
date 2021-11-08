#pragma once

// global resources that are not changed in rednering threads.
// shader code, meshes, etc.
class ShadedPathEngine;
class GlobalRendering
{
private:
	ShadedPathEngine& engine;

public:
	GlobalRendering(ShadedPathEngine& s) : engine(s) {
		//
	};
	void initiateShader_Triangle();
	Files files;
};

