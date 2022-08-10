#version 460
#extension GL_EXT_debug_printf : enable

layout (location = 0) out vec2 outUV;

void main()
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	//debugPrintfEXT("gl_VertexIndex: %d\n", gl_VertexIndex);
	//debugPrintfEXT("outUV: %f %f\n", outUV.x, outUV.y);
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
    //debugPrintfEXT("gl_Position: %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z);
}