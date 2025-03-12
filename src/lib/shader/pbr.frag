#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable

struct UBOParams {
	vec4 lightDir;
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
	float debugViewInputs;
	float debugViewEquation;
	float pad0;
	float pad1;
};

#include "shadermaterial.glsl"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;
layout (location = 5) flat in uint baseColorIndex;
layout (location = 6) flat in uint metallicRoughnessIndex;
layout (location = 7) flat in uint normalIndex;
layout (location = 8) flat in uint occlusionIndex;
layout (location = 9) flat in uint emissiveIndex;
layout (location = 10) flat in uint mode;
// mode 0: pbr metallic roughness
// mode 1: only use vertex color

struct PBRTextureIndexes {
    uint baseColor;
    uint metallicRoughness;
    uint normal;
    uint occlusion;
    uint emissive;
    uint pad0;
    uint pad1;
    uint pad2;
};

#define MAX_NUM_JOINTS 128

// info for this model instance
// see 	struct PBRTextureIndexes and struct DynamicModelUBO in pbrShader.h
// one element of the large object material buffer (descriptor updated for each model group before rednering)
layout (binding = 1) uniform UboInstance {
    mat4 model; 
    mat4 jointMatrix[MAX_NUM_JOINTS];
    uint jointcount;
    uint pad0;
    uint pad1;
    uint pad2;
    //uint padding[2]; // 8 bytes of padding to align the next member to 16 bytes
    PBRTextureIndexes indexes;
    UBOParams params;
    ShaderMaterial material;
} model_ubo;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform samplerCube global_textures3d[];
layout(set = 1, binding = 0) uniform sampler2D global_textures2d[];

vec4 textureBindless3D(uint textureid, vec3 uv) {
	return texture(global_textures3d[nonuniformEXT(textureid)], uv);
}

vec4 textureBindless2D(uint textureid, vec2 uv) {
	return texture(global_textures2d[nonuniformEXT(textureid)], uv);
}

UBOParams uboParams = model_ubo.params;

// Encapsulate the various inputs used by the various functions in the shading equation
// We store values in this struct to simplify the integration of alternative implementations
// of the shading terms, outlined in the Readme.MD Appendix.
struct PBRInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float metalness;              // metallic value at the surface
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	vec3 reflectance90;           // reflectance color at grazing angle
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	vec3 diffuseColor;            // color contribution from diffuse lighting
	vec3 specularColor;           // color contribution from specular lighting
};

const float M_PI = 3.141592653589793;
const float c_MinRoughness = 0.04;

const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 0.0;
const float PBR_WORKFLOW_SPECULAR_GLOSSINESS = 1.0;

#include "tonemapping.glsl"
#include "srgbtolinear.glsl"

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getNormal(ShaderMaterial material)
{
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = textureBindless2D(material.normalTextureSet, material.texCoordSets.normal == 0 ? inUV0 : inUV1).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inUV0);
	vec2 st2 = dFdy(inUV0);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main() {
	ShaderMaterial material = model_ubo.material;
    float f = uboParams.gamma;
    //debugPrintfEXT("uboParams.gamma %f\n", f);
    f = material.roughnessFactor;
    //debugPrintfEXT("frag material.roughnessFactor %f\n", f);
    //debugPrintfEXT("frag base set indexes direct / ubo:  %d / %d\n", baseColorIndex, material.baseColorTextureSet);
	f = material.alphaMask;
	//debugPrintfEXT("alphaMask %f\n", f);
	f = material.workflow;
	//debugPrintfEXT("workflow %f\n", f);
	uint u0 = material.texCoordSets.baseColor;
	uint u1 = material.texCoordSets.metallicRoughness;
	uint u2 = material.texCoordSets.specularGlossiness;
	//debugPrintfEXT("coord sets %d %d %d\n", u0, u1, u2);
	vec3 w = inWorldPos;
	//debugPrintfEXT("world pos %f %f %f\n", w.x, w.y, w.z);
    f = uboParams.debugViewEquation;
    //debugPrintfEXT("frag uboParams.debugvieweq %f\n", f);
    f = uboParams.gamma;
    //debugPrintfEXT("frag uboParams.gamma %f\n", f);
    //debugPrintfEXT("pbr frag render mode: %d\n", mode);
    uint baseIndex = material.baseColorTextureSet;//emissiveIndex;//baseColorIndex; // test indexes
    //uint baseIndex = baseColorIndex;
    //if (occlusionIndex == -1) { // just a test :-)
    //    baseIndex = 0;
    //}
    if (mode == 1) {
		outColor = inColor0;
        return;
	} 

//    outColor = textureBindless2D(baseIndex, inUV0) * inColor0;
//    // discard transparent pixels (== do not write z-buffer)
//    if (outColor.w < 0.8) {
//        discard;
//    }
//	return;


    // from https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/material_pbr.frag
	float perceptualRoughness;
	float metallic;
	vec3 diffuseColor;
	vec4 baseColor;

	vec3 f0 = vec3(0.04);

	if (material.alphaMask == 1.0f) {
		if (material.baseColorTextureSet > -1) {
			// we only handle metallic roughness workflow, so we can simplify the next line
			// baseColor = SRGBtoLINEAR(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;
			baseColor = SRGBtoLINEAR(textureBindless2D(material.baseColorTextureSet, material.texCoordSets.baseColor == 0 ? inUV0 : inUV1)) * material.baseColorFactor;
		} else {
			baseColor = material.baseColorFactor;
		}
		if (baseColor.a < material.alphaMaskCutoff) {
			discard;
		}
	}

	if (true /*material.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS*/) { // always true
		// Metallic and Roughness material properties are packed together
		// In glTF, these factors can be specified by fixed scalar values
		// or from a metallic-roughness map
		perceptualRoughness = material.roughnessFactor;
		metallic = material.metallicFactor;
		if (material.physicalDescriptorTextureSet > -1) {
			// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
			// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
			vec4 mrSample = textureBindless2D(material.physicalDescriptorTextureSet, material.texCoordSets.metallicRoughness == 0 ? inUV0 : inUV1);
			perceptualRoughness = mrSample.g * perceptualRoughness;
			metallic = mrSample.b * metallic;
		} else {
			perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
			metallic = clamp(metallic, 0.0, 1.0);
		}
		// Roughness is authored as perceptual roughness; as is convention,
		// convert to material roughness by squaring the perceptual roughness [2].

		// The albedo may be defined from a base texture or a flat color
		if (material.baseColorTextureSet > -1) {
			baseColor = SRGBtoLINEAR(textureBindless2D(material.baseColorTextureSet, material.texCoordSets.baseColor == 0 ? inUV0 : inUV1)) * material.baseColorFactor;
		} else {
			baseColor = material.baseColorFactor;
		}
	}

	baseColor *= inColor0;

	diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
	diffuseColor *= 1.0 - metallic;
		
	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	vec3 specularColor = mix(f0, baseColor.rgb, metallic);

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	vec3 n = (material.normalTextureSet > -1) ? getNormal(material) : normalize(inNormal);
	n.y *= -1.0f;
	vec3 v;// = normalize(ubo.camPos - inWorldPos);    // Vector from surface point to camera
	vec3 l = normalize(uboParams.lightDir.xyz);     // Vector from surface point to light
	vec3 h = normalize(l+v);                        // Half vector between both l and v
	vec3 reflection = normalize(reflect(-v, n));

	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);

	outColor = baseColor;
}