#version 450
#extension GL_EXT_debug_printf:enable
#extension GL_KHR_vulkan_glsl:enable

layout (location=0) in vec3 dir;

layout (location=0) out vec4 out_FragColor;

layout (binding=1) uniform samplerCube texture1;

void main()
{
    vec3 dir2 = vec3(1.0, 1.0, 1.0);
    //debugPrintfEXT("Cube dir: %f %f %f\n", dir.x, dir.y, dir.z);
    //debugPrintfEXT("Cube col tex: %f %f %f\n", out_FragColor.x, out_FragColor.y, out_FragColor.z);
	out_FragColor = texture(texture1, dir);
    //out_FragColor = vec4( 1.0, 1.0, 1.0, 1.0);
    //debugPrintfEXT("Cube col: %f %f %f %f\n", out_FragColor.x, out_FragColor.y, out_FragColor.z, out_FragColor.w);
}
