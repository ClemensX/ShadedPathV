# TextureAnalyzer Helper Class

## Overview
`TextureAnalyzer` is a utility class for analyzing texture graphics data in GLTF tests. It reads texture data directly from GPU memory and provides statistical analysis of color distributions, validation of expected values, and histogram generation.

## Features

### 1. Color Statistics
Computes comprehensive statistics for texture data:
- **Mean color** (average RGBA values)
- **Min/Max values** per channel
- **Standard deviation** per channel
- **Histogram** (256 bins per channel)
- **Pixel count**

### 2. Analysis Methods

#### Analyze from TextureInfo (Recommended)
```cpp
auto* texInfo = engine->textureStore.getTextureByIndex(textureIndex);
auto stats = TextureAnalyzer::analyzeTexture(engine, texInfo);
```

This method reads texture data directly from the GPU (Vulkan image) by:
1. Creating a staging buffer
2. Copying the image data from GPU to CPU memory
3. Analyzing the pixel data
4. Cleaning up resources

#### Analyze from Raw Data
```cpp
// For RGBA8 textures
auto stats = TextureAnalyzer::analyzeRGBA8(data, width, height);

// For RGB8 textures
auto stats = TextureAnalyzer::analyzeRGB8(data, width, height);
```

### 3. Validation Functions

#### Validate Color Range
Check if texture has expected average color within tolerance:
```cpp
glm::vec4 expectedMean(0.5f, 0.5f, 0.5f, 1.0f); // Gray with full alpha
auto result = TextureAnalyzer::validateColorRange(stats, expectedMean, 0.1f);
EXPECT_TRUE(result.passed) << result.message;
```

#### Validate Variance
Check if texture has expected variance (useful for detail/noise validation):
```cpp
// Expect moderate variance (not solid color, not too noisy)
auto result = TextureAnalyzer::validateVariance(stats, 0.05f, 0.3f);
EXPECT_TRUE(result.passed) << result.message;
```

#### Check Solid Color
Determine if texture is mostly uniform:
```cpp
bool isSolid = TextureAnalyzer::isSolidColor(stats, 0.01f);
if (isSolid) {
	Log("Texture is a solid color\n");
}
```

#### Validate Dominant Color
Check if a specific color dominates the texture:
```cpp
glm::vec3 redColor(1.0f, 0.0f, 0.0f);
auto result = TextureAnalyzer::validateDominantColor(stats, redColor, 0.6f);
EXPECT_TRUE(result.passed) << result.message; // Expect 60% red pixels
```

### 4. Debugging
Print statistics for inspection:
```cpp
TextureAnalyzer::printStats(stats, "BaseColor Texture");
```

Output example:
```
BaseColor Texture Statistics:
  Pixel count: 262144
  Mean (RGBA): 0.523, 0.487, 0.512, 1.0
  Min  (RGBA): 0.0, 0.0, 0.0, 1.0
  Max  (RGBA): 1.0, 1.0, 1.0, 1.0
  StdDev (RGB): 0.145, 0.132, 0.156
```

## Complete Example

```cpp
// In your test at line 186+
auto* texInfo = engine->textureStore.getTextureByIndex(
	static_cast<uint32_t>(material->baseColor));

// Analyze texture (reads from GPU automatically)
auto stats = TextureAnalyzer::analyzeTexture(engine, texInfo);
TextureAnalyzer::printStats(stats, "BaseColor");

// Validate white-ish texture
glm::vec4 expectedWhite(0.9f, 0.9f, 0.9f, 1.0f);
auto colorCheck = TextureAnalyzer::validateColorRange(stats, expectedWhite, 0.15f);
EXPECT_TRUE(colorCheck.passed) << colorCheck.message;

// Ensure it has some detail (not solid)
auto varianceCheck = TextureAnalyzer::validateVariance(stats, 0.05f, 0.5f);
EXPECT_TRUE(varianceCheck.passed) << varianceCheck.message;

// Check if red channel dominates
if (stats.mean.r > stats.mean.g && stats.mean.r > stats.mean.b) {
	Log("Red channel is dominant\n");
}

// Verify it's not a solid color
EXPECT_FALSE(TextureAnalyzer::isSolidColor(stats));
```

## Use Cases

1. **Validate Texture Loading**: Ensure loaded textures have expected properties
2. **Detect Corrupt Textures**: Identify textures with unexpected color ranges
3. **Test Color Conversions**: Verify color space conversions are correct
4. **Normal Map Validation**: Check if normal maps have typical blue-ish color
5. **Metallic/Roughness Validation**: Ensure PBR textures have expected grayscale ranges
6. **Regression Testing**: Detect unexpected changes in texture data

## Technical Implementation

### GPU Readback Process
The analyzer reads texture data from GPU using the following Vulkan operations:
1. Creates a staging buffer (host-visible memory)
2. Transitions image layout to `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`
3. Copies image data from GPU to staging buffer using `vkCmdCopyImageToBuffer`
4. Transitions image back to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
5. Maps staging buffer memory and copies data to CPU vector
6. Cleans up temporary resources

This approach works regardless of how the texture was loaded (KTX file, GLTF embedded, procedural).

### Key Implementation Details
- All color values are normalized to [0.0, 1.0] range
- Histograms use 256 bins for 8-bit data
- Standard deviation uses population formula (divides by N, not N-1)
- Only first mipmap level (level 0) is analyzed
- Supports RGB8 and RGBA8 formats
- Uses `beginSingleTimeCommandsIdle()` for command submission

## Limitations

- Only supports 8-bit per channel textures (RGB8/RGBA8)
- Analyzes only the first mipmap level
- Histogram generation assumes 8-bit data (256 bins)
- GPU readback has overhead - use sparingly in performance-critical tests

## Future Enhancements

Potential additions:
- Support for HDR formats (R32G32B32A32_SFLOAT)
- Multi-mipmap analysis
- Compressed format support (BC7, etc.)
- Spatial analysis (edge detection, frequency analysis)
- Texture comparison (diff two textures)
- More sophisticated color space analysis (HSV, Lab)
