#include "pch.h"

void GlobalRendering::initiateShader_Triangle()
{
	vector<byte> file_buffer;
	files.readFile("triangle_frag.spv", file_buffer, FileCategory::FX);
}
