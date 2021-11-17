#pragma once
class Shaders
{
public:
	Shaders(ShadedPathEngine& s) : engine(s) {
		Log("Shaders c'tor\n");
	};
	~Shaders() {
		Log("Shaders destructor\n");
	};
	void initiateShader_Triangle();
	void drawFrame_Triangle();
	bool shouldClose();

private:
	ShadedPathEngine& engine;
};

