/* Copyright (c) 2018-2023, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct TexCoordSets {
	uint baseColor;
	uint metallicRoughness;
	uint specularGlossiness;
	uint normal;
	uint occlusion;
	uint emissive;
};

struct ShaderMaterial {
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	vec4 diffuseFactor;
	vec4 specularFactor;
	float workflow;
	int baseColorTextureSet;
	int physicalDescriptorTextureSet;
	int normalTextureSet;	
	int occlusionTextureSet;
	int emissiveTextureSet;
	int brdflut;
	int irradiance;
	int envcube;
	float metallicFactor;	
	float roughnessFactor;	
	float alphaMask;	
	float alphaMaskCutoff;
	float emissiveStrength;
	uint pad0;
	uint pad1;
	TexCoordSets texCoordSets;
};