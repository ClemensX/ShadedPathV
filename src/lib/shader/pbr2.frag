#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_mesh_shader : enable
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : require


#include "common_cpp_shader.h"
//#include "shadermaterial.glsl"
#include "pbr_mesh_common2.glsl"

layout(location = 0) flat in PBRVertexOutFlat inVertFlat;
layout(location = 1) in PBRVertexOut inVert; // flat removed

GPUModel model = gpuModels.model[pushConstants.objectNum];
GPUMeshInfo mesh = gpuInfos.info[model.meshNumber];
GPUMaterial material = gpuMaterials.material[mesh.material];
GPUFrameParam uboParams = gpuFrameParams.frameParam[0];

// mode 0: pbr metallic roughness
// mode 1: only use vertex color

vec2 inUV0 = inVert.uv0;
vec2 inUV1 = inVert.uv1;
vec3 inWorldPos = inVert.worldPos;
vec3 inNormal = inVert.normal;
vec3 camPos = ubo.camPos;
vec4 inColor0 = inVert.color0;
float inpad0 = inVert.pad0;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform samplerCube global_textures3d[];
layout(set = 1, binding = 0) uniform sampler2D global_textures2d[];

vec4 textureBindless3DLod(uint textureid, vec3 uv, float lod) {
	vec3 myuv = uv;
	//myuv.y = 1.0 - myuv.y;
	return textureLod(global_textures3d[nonuniformEXT(textureid)], myuv, lod);
}

vec4 textureBindless3D(uint textureid, vec3 uv) {
	return texture(global_textures3d[nonuniformEXT(textureid)], uv);
}

vec4 textureBindless2D(uint textureid, vec2 uv) {
	vec2 wrappedUV = uv;
	return texture(global_textures2d[nonuniformEXT(textureid)], wrappedUV);
}

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
vec3 getNormalNaN(GPUMaterial material)
{
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = textureBindless2D(material.normalTextureSet, material.coord_set_normal == 0 ? inUV0 : inUV1).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inUV0);
	vec2 st2 = dFdy(inUV0);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
	//return normalize(N);
}

// fixed getNormal():
vec3 safeNormalize(vec3 v, vec3 fallback)
{
	float len2 = dot(v, v);
	return (len2 > 1e-12) ? v * inversesqrt(len2) : fallback;
}

vec3 getNormal(GPUMaterial material)
{
	vec3 N = safeNormalize(inNormal, vec3(0.0, 0.0, 1.0));

	if (material.normalTextureSet < 0) {
		return N;
	}

	vec2 uv = (material.coord_set_normal == 0u) ? inUV0 : inUV1;

	vec3 tangentNormal = textureBindless2D(material.normalTextureSet, uv).xyz * 2.0 - 1.0;
	tangentNormal = safeNormalize(tangentNormal, vec3(0.0, 0.0, 1.0));

	// If the normal map looks vertically inverted, enable this:
	//tangentNormal.y = -tangentNormal.y;

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(uv);
	vec2 st2 = dFdy(uv);

	vec3 Traw = q1 * st2.y - q2 * st1.y;
	vec3 Braw = -q1 * st2.x + q2 * st1.x;

	float tLen2 = dot(Traw, Traw);
	float bLen2 = dot(Braw, Braw);

	if (tLen2 <= 1e-12 || bLen2 <= 1e-12) {
		return N;
	}

	vec3 T = normalize(Traw - N * dot(N, Traw));
	vec3 B = normalize(Braw - N * dot(N, Braw));

	// Keep a consistent handedness
	if (dot(cross(N, T), B) < 0.0) {
		B = -B;
	}

	mat3 TBN = mat3(T, B, N);
	return safeNormalize(TBN * tangentNormal, N);
}

// Add near other constants
const int IBL_DEBUG_DIR_MODE = 0;
// 0 = reflect(-v,n)    (expected physical)
// 1 = -reflect(-v,n)
// 2 = v
// 3 = -v
// 4 = n

const vec3 IBL_CUBE_DIR_SIGN = vec3(1.0, 1.0, 1.0); // test axis flips here

vec3 fixIblCubeDir(vec3 d)
{
	return normalize(d * IBL_CUBE_DIR_SIGN);
}

vec3 pickIblDir(vec3 n, vec3 v)
{
	vec3 r = normalize(reflect(-v, n));
	if (IBL_DEBUG_DIR_MODE == 1) r = -r;
	else if (IBL_DEBUG_DIR_MODE == 2) r = normalize(v);
	else if (IBL_DEBUG_DIR_MODE == 3) r = normalize(-v);
	else if (IBL_DEBUG_DIR_MODE == 4) r = normalize(n);
	return normalize(r * IBL_CUBE_DIR_SIGN);
}

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection, GPUMaterial material)
{
//	vec3 nIbl = fixIblCubeDir(n);
//	vec3 rIbl = fixIblCubeDir(reflection);
	vec3 nIbl = n;
	vec3 rIbl = reflection;

	float lod = (pbrInputs.perceptualRoughness * uboParams.prefilteredCubeMipLevels);
	// retrieve a scale and bias to F0. See [1], Figure 3
	//textureBindless2D(material.baseColorTextureSet
	//n.y -= n.y;
	vec3 brdf = (textureBindless2D(uboParams.brdflut, vec2(pbrInputs.NdotV, 1.0 - pbrInputs.perceptualRoughness))).rgb;
	vec3 diffuseLight = SRGBtoLINEAR(tonemap(textureBindless3D(uboParams.irradiance, nIbl))).rgb;

	vec3 myref = reflection;
	//myref.y = -myref.y;
	vec3 specularLight = SRGBtoLINEAR(tonemap(textureBindless3DLod(uboParams.envcube, rIbl, lod))).rgb;
	//specularLight = vec3(0.0); // disable IBL for now

	vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;
	vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);

	// For presentation, this allows us to disable IBL terms
	// For presentation, this allows us to disable IBL terms
	diffuse *= uboParams.scaleIBLAmbient;
	specular *= uboParams.scaleIBLAmbient;

	return diffuse + specular;
}

// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
vec3 diffuse(PBRInfo pbrInputs)
{
	return pbrInputs.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbrInputs)
{
	return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs)
{
	float NdotL = pbrInputs.NdotL;
	float NdotV = pbrInputs.NdotV;
	float r = pbrInputs.alphaRoughness;

	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs)
{
	float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
	float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
	return roughnessSq / (M_PI * f * f);
}

void test() {
	//verifyTextures(material);
    //verifyMaterial(0);
//	if (material.baseColorTextureSet > 0) {
//		outColor = vec4(0.1, 1, 0.1, 0.6);
//	} else if (material.lod_category == 42) {
//		outColor = vec4(0.1, 0.1, 1, 0.6);
//	} else {
//		outColor = vec4(1, 0.1, 0.1, 0.6);
//	}
    debugPrintfEXT("pbr frag brdflut %d , env %d (levels %f), irr %d, ibl ambient %f\n", uboParams.brdflut, uboParams.envcube, uboParams.prefilteredCubeMipLevels, uboParams.irradiance, uboParams.scaleIBLAmbient);
}

void main() {
	//test();

	if ((model.flags & MODEL_RENDER_FLAG_USE_VERTEX_COLORS) != 0) {
		outColor = vec4(1, 1, 1, 1);
		outColor = inColor0;
		//debugPrintfEXT("pbr frag MODEL_RENDER_FLAG_USE_VERTEX_COLORS\n");
        return;
	} 
    // from https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/material_pbr.frag
	float perceptualRoughness;
	float metallic;
	vec3 diffuseColor;
	vec4 baseColor = vec4(1.0);

	vec3 f0 = vec3(0.04);

	if (false) {
		// for debugging: return full texture
		if (material.baseColorTextureSet > -1 && false) {
			baseColor = textureBindless2D(material.baseColorTextureSet, material.coord_set_baseColor == 0 ? inUV0 : inUV1);
		}
		if (material.emissiveTextureSet > -1 && false) {
			baseColor = textureBindless2D(material.emissiveTextureSet, material.coord_set_emissive == 0 ? inUV0 : inUV1);
		}
		if (material.normalTextureSet > -1 && false) {
			baseColor = textureBindless2D(material.normalTextureSet, material.coord_set_normal == 0 ? inUV0 : inUV1);
		}
		if (material.occlusionTextureSet > -1 && false) {
			baseColor = textureBindless2D(material.occlusionTextureSet, material.coord_set_occlusion == 0 ? inUV0 : inUV1);
			baseColor.g = baseColor.b = 0.0;
		}
		if (material.physicalDescriptorTextureSet > -1 && true) {
			baseColor = textureBindless2D(material.physicalDescriptorTextureSet, material.coord_set_metallicRoughness == 0 ? inUV0 : inUV1);
			baseColor.r = 0.0;
		}
		if (material.physicalDescriptorTextureSet > -1 && false) {
			// merged occ/metal/rough:
			baseColor = textureBindless2D(material.physicalDescriptorTextureSet, material.coord_set_metallicRoughness == 0 ? inUV0 : inUV1);
			vec4 occ = textureBindless2D(material.occlusionTextureSet, material.coord_set_occlusion == 0 ? inUV0 : inUV1);
			baseColor.r = occ.r;
		}
		outColor = baseColor;
		return;
	}

	if (material.alphaMask == 1.0f) {
		if (material.baseColorTextureSet > -1) {
			// we only handle metallic roughness workflow, so we can simplify the next line
			// baseColor = SRGBtoLINEAR(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;
			// linearization is done automatically for sRGB formats
			//baseColor = SRGBtoLINEAR(textureBindless2D(material.baseColorTextureSet, material.texCoordSets.baseColor == 0 ? inUV0 : inUV1)) * material.baseColorFactor;
			baseColor = textureBindless2D(material.baseColorTextureSet, material.coord_set_baseColor == 0 ? inUV0 : inUV1) * material.baseColorFactor;
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
			vec4 mrSample = textureBindless2D(material.physicalDescriptorTextureSet, material.coord_set_metallicRoughness == 0 ? inUV0 : inUV1);
			perceptualRoughness = mrSample.g * perceptualRoughness;
			metallic = mrSample.b * metallic;
			//if (metallic < 0.7) debugPrintfEXT("metallic %f\n", metallic);
			//if (mrSample.b < 0.7) debugPrintfEXT("metallic %f sample %f rough %f\n", metallic, mrSample.b, perceptualRoughness);
//			outColor = vec4(mrSample.b, 0, 0, 1);
//			return;
		} else {
			perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
			metallic = clamp(metallic, 0.0, 1.0);
		}

		// Roughness is authored as perceptual roughness; as is convention,
		// convert to material roughness by squaring the perceptual roughness [2].

		// The albedo may be defined from a base texture or a flat color
		if (material.baseColorTextureSet > -1) {
			vec4 baseColorIn = textureBindless2D(material.baseColorTextureSet, material.coord_set_baseColor == 0 ? inUV0 : inUV1);
			// linearization is done automatically for sRGB formats
			//baseColor = SRGBtoLINEAR(baseColorIn) * material.baseColorFactor;
			baseColor = baseColorIn * material.baseColorFactor;
//			float sf = 5.0; // use higher value for light boost, TODO move to C++ code
//			vec4 f = vec4(sf, sf, sf, 1.0);
//			baseColor = SRGBtoLINEAR(textureBindless2D(material.baseColorTextureSet, material.texCoordSets.baseColor == 0 ? inUV0 : inUV1)) * material.baseColorFactor; // * f;
			//debugPrintfEXT("pbr baseColor factor %f %f %f %f\n", material.baseColorFactor.r, material.baseColorFactor.g, material.baseColorFactor.b, material.baseColorFactor.a);
			//debugPrintfEXT("pbr frag baseColor %f %f %f %f with factor %f\n", baseColor.r, baseColor.g, baseColor.b, baseColor.a, material.baseColorFactor.r);
		} else {
			baseColor = material.baseColorFactor;
		}
	}

	baseColor *= inColor0;
//	outColor = baseColor;
//	return;

	diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
	diffuseColor *= 1.0 - metallic;
		
	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	vec3 specularColor = mix(f0, baseColor.rgb, metallic);

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
	//debugPrintfEXT("reflectance %f\n", reflectance);

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	//debugPrintfEXT("frag camPos: %f %f %f\n", camPos.x, camPos.y, camPos.z);
	//debugPrintfEXT("frag inWorldPos: %f %f %f\n", inWorldPos.x, inWorldPos.y, inWorldPos.z);
	vec3 n = (material.normalTextureSet > -1) ? getNormal(material) : normalize(inNormal);
	n.y *= -1.0f;
	vec3 v = normalize(camPos - inWorldPos);    // Vector from surface point to camera
	vec3 l = normalize(uboParams.lightDir.xyz);     // Vector from surface point to light
	vec3 h = normalize(l+v);                        // Half vector between both l and v
	vec3 reflection = normalize(reflect(-v, n));
	//reflection = pickIblDir(n, v);
	//reflection.y = -reflection.y;

	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);

	PBRInfo pbrInputs = PBRInfo(
		NdotL,
		NdotV,
		NdotH,
		LdotH,
		VdotH,
		perceptualRoughness,
		metallic,
		specularEnvironmentR0,
		specularEnvironmentR90,
		alphaRoughness,
		diffuseColor,
		specularColor
	);

	// Calculate the shading terms for the microfacet specular shading model
	vec3 F = specularReflection(pbrInputs);
	float G = geometricOcclusion(pbrInputs);
	float D = microfacetDistribution(pbrInputs);

	vec3 u_LightColor = vec3(1.0) * uboParams.intensity;
	//debugPrintfEXT("frag uboParams.intensity %f:\n", uboParams.intensity);

	// Calculation of analytical lighting contribution
	vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
	vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
	// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
	vec3 color = NdotL * u_LightColor * (diffuseContrib + specContrib);
//	outColor = vec4(color, baseColor.a);
//	return;

	// Calculate lighting contribution from image based lighting source (IBL)
	vec3 iblcolor = getIBLContribution(pbrInputs, n, reflection, material);
	color += iblcolor;
//	outColor = vec4(baseColor.rgb, baseColor.a);
//	return;

	const float u_OcclusionStrength = 1.0f;
	// Apply optional PBR terms for additional (optional) shading
	if (material.occlusionTextureSet > -1) {
		float ao = textureBindless2D(material.occlusionTextureSet, (material.coord_set_occlusion == 0 ? inUV0 : inUV1)).r;
		color = mix(color, color * ao, u_OcclusionStrength);
	}

	vec3 emissive = material.emissiveFactor.rgb * material.emissiveStrength;
	//debugPrintfEXT("frag material.emissiveFactor.rgb: %f %f %f\n", material.emissiveFactor.rgb.r, material.emissiveFactor.rgb.g, material.emissiveFactor.rgb.b);
	//debugPrintfEXT("     material.emissiveStrength %f:\n", material.emissiveStrength);

	if (material.emissiveTextureSet > -1) {
		vec3 em = SRGBtoLINEAR(textureBindless2D(material.emissiveTextureSet, material.coord_set_emissive == 0 ? inUV0 : inUV1)).rgb;
		//debugPrintfEXT("frag emissive texture: %f %f %f\n", em.r, em.g, em.b);
		emissive *= em;
	};
	color += emissive;
	
	outColor = vec4(color, baseColor.a);

	//outColor = vec4(1, 1, 1, 1);
	//outColor = baseColor;
}
