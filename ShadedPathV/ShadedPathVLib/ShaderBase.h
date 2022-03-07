#pragma once
class ShaderBase
{
public:
	ShaderBase() {

	};

	virtual ~ShaderBase() = 0;

	// initializations: RenderPass etc.
	virtual void init(ShadedPathEngine& engine) = 0;
};

