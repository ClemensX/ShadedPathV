#version 460
#extension GL_EXT_debug_printf:enable
#extension GL_KHR_vulkan_glsl:enable

layout (points) in;
layout( triangle_strip, max_vertices = 4 ) out;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 1) in uint inTypes[]; // billboard type: 0 is towards camera, 1 is absolute inDirection
layout(location = 2) in vec4 inQuats[]; // quaternion for rotating vertices if type == 1

void printMatrix(mat4 m) {
    debugPrintfEXT("matrix 1.row: %f %f %f %f\n", m[0][0], m[0][1], m[0][2], m[0][3]);
    debugPrintfEXT("matrix 2.row: %f %f %f %f\n", m[1][0], m[1][1], m[1][2], m[1][3]);
    debugPrintfEXT("matrix 3.row: %f %f %f %f\n", m[2][0], m[2][1], m[2][2], m[2][3]);
    debugPrintfEXT("matrix 4.row: %f %f %f %f\n", m[3][0], m[3][1], m[3][2], m[3][3]);
}

vec3 apply_quaternion_to_position(vec4 quat, vec4 position)
{ 
  vec4 q = quat;
  vec3 v = position.xyz;
  v = v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
  v.y *= -1;
  return v;
}

void calcVertsOrigin(inout vec3 array[3])
{
	array[0] = vec3(-0.1, 0, 0);
	array[1] = vec3(0.1, 0, 0);
	array[2] = vec3(0, 0.1, 0);
}

void main()
{
    vec3 verts[3];
    calcVertsOrigin(verts);
    uint inType = inTypes[0];
    vec4 quat = inQuats[0];
    vec4 inP = gl_in[0].gl_Position;
    //debugPrintfEXT("bb geom.input x y z is %f %f %f\n", inP.x, inP.y, inP.z);
    //debugPrintfEXT("bb geom ubo.model 0 0 is %f\n", ubo.model[0][0]);
    //debugPrintfEXT("bb geom ubo.view 0 0 is %f\n", ubo.view[0][0]);

    vec4 v0, v1, v2; // assemble vertex info here
    if (inType == 0) {
        v0 = vec4(verts[0], 0) + inP;
        v1 = vec4(verts[1], 0) + inP;
        v2 = vec4(verts[2], 0) + inP;
        gl_Position = ubo.proj * v0;
        EmitVertex();
        gl_Position = ubo.proj * v1;
        EmitVertex();
        gl_Position = ubo.proj * v2;
        EmitVertex();
        EndPrimitive();
    } else if (inType == 1) {
        //debugPrintfEXT("bb geom.quat w x y z is %f %f %f %f\n", quat.w, quat.x, quat.y, quat.z);
        v0 = vec4(verts[0], 0);
        v1 = vec4(verts[1], 0);
        v2 = vec4(verts[2], 0);
        vec3 v0_ = apply_quaternion_to_position(quat, v0);
        vec3 v1_ = apply_quaternion_to_position(quat, v1);
        vec3 v2_ = apply_quaternion_to_position(quat, v2);
        v0 = vec4(v0_, 0) + inP;
        v1 = vec4(v1_, 0) + inP;
        v2 = vec4(v2_, 0) + inP;
        //debugPrintfEXT("bb geom.v1 x y z is %f %f %f %f\n", v1.x, v1.y, v1.z, v1.w);
        //printMatrix(ubo.proj);
        v0 = ubo.proj * ubo.view * ubo.model * v0;
        v1 = ubo.proj * ubo.view * ubo.model * v1;
        v2 = ubo.proj * ubo.view * ubo.model * v2;
        gl_Position = v0;
        EmitVertex();
        gl_Position = v1;
        //debugPrintfEXT("bb geom.v1 final x y z is %f %f %f\n", v1.x, v1.y, v1.z);
        EmitVertex();
        gl_Position = v2;
        EmitVertex();
        EndPrimitive();
    }
}