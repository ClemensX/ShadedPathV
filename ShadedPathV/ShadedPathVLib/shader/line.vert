#version 450
#extension GL_EXT_debug_printf : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;


layout(location = 0) out vec3 fragColor;

void printMatrix(mat4 m) {
    debugPrintfEXT("matrix 1.row: %f %f %f %f\n", m[0][0], m[0][1], m[0][2], m[0][3]);
    debugPrintfEXT("matrix 2.row: %f %f %f %f\n", m[1][0], m[1][1], m[1][2], m[1][3]);
    debugPrintfEXT("matrix 3.row: %f %f %f %f\n", m[2][0], m[2][1], m[2][2], m[2][3]);
    debugPrintfEXT("matrix 4.row: %f %f %f %f\n", m[3][0], m[3][1], m[3][2], m[3][3]);
}

void main() {
    //debugPrintfEXT("input vertex world coord: %f %f %f\n", inPosition.x, inPosition.y, inPosition.z);
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    // -0.200000 0.000000 -0.100000
//    if (inPosition.x == -0.2 && inPosition.y == 0 && inPosition.z == -0.1) {
//        debugPrintfEXT("input vertex world coord: %f %f %f\n", inPosition.x, inPosition.y, inPosition.z);
//        printMatrix(ubo.proj);
//        debugPrintfEXT("output vertex: %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z);
//    }
    //debugPrintfEXT("ubo.model 0 0 is %f\n", ubo.model[0][0]);
    //debugPrintfEXT("ubo.view 0 0 is %f\n", ubo.view[0][0]);
    //debugPrintfEXT("ubo.proj 0 0 is %f\n", ubo.proj[0][0]);

    fragColor = inColor;
    //fragColor = vec3(1, 1, 1);
    //debugPrintfEXT("final device coord: %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z);
}