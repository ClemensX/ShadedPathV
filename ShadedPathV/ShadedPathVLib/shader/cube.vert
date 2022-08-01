#version 460
#extension GL_EXT_debug_printf:enable
#extension GL_KHR_vulkan_glsl:enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	float bloat;
	bool outside;
} ubo;

layout(location = 0) in vec3 inPosition;
layout (location=0) out vec3 dir;

const vec3 pos[8] = vec3[8](
	vec3(-1.0,-1.0, 1.0),
	vec3( 1.0,-1.0, 1.0),
	vec3( 1.0, 1.0, 1.0),
	vec3(-1.0, 1.0, 1.0),

	vec3(-1.0,-1.0,-1.0),
	vec3( 1.0,-1.0,-1.0),
	vec3( 1.0, 1.0,-1.0),
	vec3(-1.0, 1.0,-1.0)
);

const int indices[36] = int[36](
	// front
	0, 2, 1, 2, 0, 3,
	// right
	1, 6, 5, 6, 1, 2,
	// back
	7, 5, 6, 5, 7, 4,
	// left
	4, 3, 0, 3, 4, 7,
	// bottom
	4, 1, 5, 1, 4, 0,
	// top
	3, 6, 2, 6, 3, 7
);

void main()
{
    //debugPrintfEXT("ubo.model 0 0 is %f\n", ubo.model[0][0]);
    //debugPrintfEXT("ubo.view 0 0 is %f\n", ubo.view[0][0]);
    //debugPrintfEXT("ubo.proj 0 0 is %f\n", ubo.proj[0][0]);
	int idx = indices[gl_VertexIndex];
    //debugPrintfEXT("Cube input vertex world %d idx: %d coord: %f %f %f\n", gl_VertexIndex, idx, pos[idx].x, pos[idx].y, pos[idx].z);
	//gl_Position = ubo.proj * ubo.view * vec4(1.0 * pos[idx], 1.0);  // use for stationary cube
	gl_Position = ubo.proj * ubo.view * vec4(ubo.bloat * pos[idx], 1.0);
	//if (gl_VertexIndex == 0) debugPrintfEXT("final device coord: %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z);
	//if (gl_VertexIndex == 0) debugPrintfEXT("bloat: %f\n", ubo.bloat);
	vec3 updown = pos[idx].xyz;
	if (ubo.outside) {
		updown.y *= -1;
	}
	dir = updown.xyz;
	//dir = pos[idx].xyz;
}
