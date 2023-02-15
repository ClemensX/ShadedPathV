#version 450
#extension GL_EXT_debug_printf:enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_KHR_vulkan_glsl:enable

//layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in uint texIndex;

layout(location = 0) out vec4 outColor;

//layout (set = 0, binding = 2) uniform sampler2D baseTex;
layout(set = 1, binding = 0) uniform sampler2D global_textures[];

void main()
{
    outColor = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord);
    //vec3 dir = vec3(1.0, 1.0, 1.0);
    //debugPrintfEXT("bb frag dir: %f %f %f\n", dir.x, dir.y, dir.z);
//    //debugPrintfEXT("Cube col tex: %f %f %f\n", out_FragColor.x, out_FragColor.y, out_FragColor.z);
//    vec4 outColor2 = texture(baseTex, fragTexCoord);
//    outColor = outColor2;
//    vec4 out_FragColor = vec4( 1.0, 1.0, 1.0, 1.0);
//    outColor = out_FragColor;
//    //debugPrintfEXT("Cube col: %f %f %f\n", out_FragColor.x, out_FragColor.y, out_FragColor.z);
}
