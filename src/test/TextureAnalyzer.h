#pragma once

#include "mainheader.h"
#include <algorithm>
#include <numeric>
#include <cmath>

// Helper class to analyze texture graphics data
// Can check color distributions, pixel values, and validate texture properties
class TextureAnalyzer {
public:
    struct ColorStats {
        glm::vec4 mean = glm::vec4(0.0f);      // Average color
        glm::vec4 min = glm::vec4(FLT_MAX);     // Minimum values per channel
        glm::vec4 max = glm::vec4(-FLT_MAX);    // Maximum values per channel
        glm::vec4 stddev = glm::vec4(0.0f);     // Standard deviation per channel
        size_t pixelCount = 0;

        // Histogram per channel (256 bins for 8-bit data)
        std::array<std::vector<uint32_t>, 4> histogram;

        ColorStats() {
            for (auto& hist : histogram) {
                hist.resize(256, 0);
            }
        }
    };

    struct ValidationResult {
        bool passed = true;
        std::string message;
    };

    // Analyze texture from raw RGBA8 data
    static ColorStats analyzeRGBA8(const uint8_t* data, uint32_t width, uint32_t height) {
        ColorStats stats;
        stats.pixelCount = static_cast<size_t>(width) * height;

        if (!data || stats.pixelCount == 0) {
            return stats;
        }

        // First pass: compute mean, min, max, and histogram
        glm::dvec4 sum(0.0);
        for (size_t i = 0; i < stats.pixelCount; ++i) {
            size_t idx = i * 4;
            glm::vec4 pixel(
                data[idx + 0] / 255.0f,
                data[idx + 1] / 255.0f,
                data[idx + 2] / 255.0f,
                data[idx + 3] / 255.0f
            );

            sum += glm::dvec4(pixel);
            stats.min = glm::min(stats.min, pixel);
            stats.max = glm::max(stats.max, pixel);

            // Update histogram
            stats.histogram[0][data[idx + 0]]++;
            stats.histogram[1][data[idx + 1]]++;
            stats.histogram[2][data[idx + 2]]++;
            stats.histogram[3][data[idx + 3]]++;
        }

        stats.mean = glm::vec4(sum) / static_cast<float>(stats.pixelCount);

        // Second pass: compute standard deviation
        glm::dvec4 variance(0.0);
        for (size_t i = 0; i < stats.pixelCount; ++i) {
            size_t idx = i * 4;
            glm::vec4 pixel(
                data[idx + 0] / 255.0f,
                data[idx + 1] / 255.0f,
                data[idx + 2] / 255.0f,
                data[idx + 3] / 255.0f
            );

            glm::vec4 diff = pixel - stats.mean;
            variance += glm::dvec4(diff * diff);
        }

        variance /= static_cast<double>(stats.pixelCount);
        stats.stddev = glm::vec4(glm::sqrt(variance));

        return stats;
    }

    // Analyze texture from raw RGB8 data
    static ColorStats analyzeRGB8(const uint8_t* data, uint32_t width, uint32_t height) {
        ColorStats stats;
        stats.pixelCount = static_cast<size_t>(width) * height;

        if (!data || stats.pixelCount == 0) {
            return stats;
        }

        glm::dvec4 sum(0.0);
        for (size_t i = 0; i < stats.pixelCount; ++i) {
            size_t idx = i * 3;
            glm::vec4 pixel(
                data[idx + 0] / 255.0f,
                data[idx + 1] / 255.0f,
                data[idx + 2] / 255.0f,
                1.0f  // Alpha always 1.0 for RGB
            );

            sum += glm::dvec4(pixel);
            stats.min = glm::min(stats.min, pixel);
            stats.max = glm::max(stats.max, pixel);

            stats.histogram[0][data[idx + 0]]++;
            stats.histogram[1][data[idx + 1]]++;
            stats.histogram[2][data[idx + 2]]++;
        }

        stats.mean = glm::vec4(sum) / static_cast<float>(stats.pixelCount);

        glm::dvec4 variance(0.0);
        for (size_t i = 0; i < stats.pixelCount; ++i) {
            size_t idx = i * 3;
            glm::vec4 pixel(
                data[idx + 0] / 255.0f,
                data[idx + 1] / 255.0f,
                data[idx + 2] / 255.0f,
                1.0f
            );

            glm::vec4 diff = pixel - stats.mean;
            variance += glm::dvec4(diff * diff);
        }

        variance /= static_cast<double>(stats.pixelCount);
        stats.stddev = glm::vec4(glm::sqrt(variance));

        return stats;
    }

    // Validate that texture has expected color distribution
    static ValidationResult validateColorRange(const ColorStats& stats, 
                                                 const glm::vec4& expectedMean, 
                                                 float tolerance = 0.1f) {
        ValidationResult result;
        glm::vec4 diff = glm::abs(stats.mean - expectedMean);

        if (glm::any(glm::greaterThan(diff, glm::vec4(tolerance)))) {
            result.passed = false;
            result.message = "Color mean outside tolerance. Expected: (" +
                           std::to_string(expectedMean.r) + ", " +
                           std::to_string(expectedMean.g) + ", " +
                           std::to_string(expectedMean.b) + ", " +
                           std::to_string(expectedMean.a) + "), Got: (" +
                           std::to_string(stats.mean.r) + ", " +
                           std::to_string(stats.mean.g) + ", " +
                           std::to_string(stats.mean.b) + ", " +
                           std::to_string(stats.mean.a) + ")";
        } else {
            result.message = "Color range validation passed";
        }

        return result;
    }

    // Check if texture is mostly a solid color
    static bool isSolidColor(const ColorStats& stats, float threshold = 0.01f) {
        return glm::all(glm::lessThan(stats.stddev, glm::vec4(threshold)));
    }

    // Check if texture has expected variance (useful for noise/detail validation)
    static ValidationResult validateVariance(const ColorStats& stats, 
                                              float minVariance, 
                                              float maxVariance) {
        ValidationResult result;
        float avgStdDev = (stats.stddev.r + stats.stddev.g + stats.stddev.b) / 3.0f;

        if (avgStdDev < minVariance) {
            result.passed = false;
            result.message = "Texture variance too low: " + std::to_string(avgStdDev) +
                           " (expected min: " + std::to_string(minVariance) + ")";
        } else if (avgStdDev > maxVariance) {
            result.passed = false;
            result.message = "Texture variance too high: " + std::to_string(avgStdDev) +
                           " (expected max: " + std::to_string(maxVariance) + ")";
        } else {
            result.message = "Variance validation passed (stddev: " + 
                           std::to_string(avgStdDev) + ")";
        }

        return result;
    }

    // Validate that a specific color dominates the texture
    static ValidationResult validateDominantColor(const ColorStats& stats, 
                                                   const glm::vec3& expectedColor,
                                                   float minPercentage = 0.5f) {
        ValidationResult result;

        // Check histogram bins around expected color
        uint8_t targetR = static_cast<uint8_t>(expectedColor.r * 255.0f);
        uint8_t targetG = static_cast<uint8_t>(expectedColor.g * 255.0f);
        uint8_t targetB = static_cast<uint8_t>(expectedColor.b * 255.0f);

        // Count pixels within a small range of target color (±10)
        const int range = 10;
        size_t matchCount = 0;

        for (int r = std::max(0, targetR - range); r <= std::min(255, targetR + range); ++r) {
            for (int g = std::max(0, targetG - range); g <= std::min(255, targetG + range); ++g) {
                for (int b = std::max(0, targetB - range); b <= std::min(255, targetB + range); ++b) {
                    matchCount += std::min({stats.histogram[0][r], 
                                           stats.histogram[1][g], 
                                           stats.histogram[2][b]});
                }
            }
        }

        float percentage = static_cast<float>(matchCount) / static_cast<float>(stats.pixelCount);

        if (percentage < minPercentage) {
            result.passed = false;
            result.message = "Dominant color validation failed. Only " + 
                           std::to_string(percentage * 100.0f) + "% matches (expected min: " +
                           std::to_string(minPercentage * 100.0f) + "%)";
        } else {
            result.message = "Dominant color validation passed (" + 
                           std::to_string(percentage * 100.0f) + "% matches)";
        }

        return result;
    }

    // Print statistics (useful for debugging)
    static void printStats(const ColorStats& stats, const std::string& label = "Texture") {
        Log(label << " Statistics:\n");
        Log("  Pixel count: " << stats.pixelCount << "\n");
        Log("  Mean (RGBA): " << stats.mean.r << ", " << stats.mean.g << ", " 
            << stats.mean.b << ", " << stats.mean.a << "\n");
        Log("  Min  (RGBA): " << stats.min.r << ", " << stats.min.g << ", " 
            << stats.min.b << ", " << stats.min.a << "\n");
        Log("  Max  (RGBA): " << stats.max.r << ", " << stats.max.g << ", " 
            << stats.max.b << ", " << stats.max.a << "\n");
        Log("  StdDev (RGB): " << stats.stddev.r << ", " << stats.stddev.g << ", " 
            << stats.stddev.b << "\n");
    }

    // Read texture data from GPU Vulkan image back to CPU memory
    // This requires creating a staging buffer and copying the image data
    static std::vector<uint8_t> readTextureDataFromGPU(ShadedPathEngine* engine, 
                                                        TextureInfo* texInfo) {
        std::vector<uint8_t> data;

        if (!texInfo || !texInfo->isAvailable()) {
            Log("TextureAnalyzer: texture not available\n");
            return data;
        }

        // Get texture dimensions
        uint32_t width = texInfo->vulkanTexture.width;
        uint32_t height = texInfo->vulkanTexture.height;
        VkFormat format = texInfo->vulkanTexture.imageFormat;

        // Calculate buffer size based on format
        size_t bytesPerPixel = 4; // Assume RGBA8 for now
        size_t bufferSize = width * height * bytesPerPixel;

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(engine->globalRendering.device, &bufferInfo, nullptr, &stagingBuffer);
        if (result != VK_SUCCESS) {
            Log("TextureAnalyzer: failed to create staging buffer\n");
            return data;
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(engine->globalRendering.device, stagingBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = engine->globalRendering.findMemoryTypeIndex(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        result = vkAllocateMemory(engine->globalRendering.device, &allocInfo, nullptr, &stagingMemory);
        if (result != VK_SUCCESS) {
            vkDestroyBuffer(engine->globalRendering.device, stagingBuffer, nullptr);
            Log("TextureAnalyzer: failed to allocate staging memory\n");
            return data;
        }

        vkBindBufferMemory(engine->globalRendering.device, stagingBuffer, stagingMemory, 0);

        // Copy image to buffer
        VkCommandBuffer commandBuffer = engine->globalRendering.beginSingleTimeCommandsIdle();

        // Transition image layout to transfer source
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texInfo->vulkanTexture.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        // Copy image to buffer
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyImageToBuffer(commandBuffer, texInfo->vulkanTexture.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);

        // Transition back to shader read
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        engine->globalRendering.endSingleTimeCommandsIdle(commandBuffer);

        // Map memory and copy to vector
        void* mappedData;
        vkMapMemory(engine->globalRendering.device, stagingMemory, 0, bufferSize, 0, &mappedData);
        data.resize(bufferSize);
        memcpy(data.data(), mappedData, bufferSize);
        vkUnmapMemory(engine->globalRendering.device, stagingMemory);

        // Cleanup
        vkDestroyBuffer(engine->globalRendering.device, stagingBuffer, nullptr);
        vkFreeMemory(engine->globalRendering.device, stagingMemory, nullptr);

        return data;
    }

    // Analyze texture directly from TextureInfo by reading from GPU
    static ColorStats analyzeTexture(ShadedPathEngine* engine, TextureInfo* texInfo) {
        ColorStats stats;

        if (!texInfo || !texInfo->isAvailable()) {
            Log("TextureAnalyzer: texture not available\n");
            return stats;
        }

        // Read texture data from GPU
        std::vector<uint8_t> data = readTextureDataFromGPU(engine, texInfo);

        if (data.empty()) {
            Log("TextureAnalyzer: failed to read texture data from GPU\n");
            return stats;
        }

        uint32_t width = texInfo->vulkanTexture.width;
        uint32_t height = texInfo->vulkanTexture.height;
        VkFormat format = texInfo->vulkanTexture.imageFormat;

        // Determine format and analyze accordingly
        bool hasAlpha = (format == VK_FORMAT_R8G8B8A8_UNORM || 
                        format == VK_FORMAT_R8G8B8A8_SRGB ||
                        format == VK_FORMAT_B8G8R8A8_UNORM ||
                        format == VK_FORMAT_B8G8R8A8_SRGB);

        if (hasAlpha) {
            stats = analyzeRGBA8(data.data(), width, height);
        } else {
            stats = analyzeRGB8(data.data(), width, height);
        }

        return stats;
    }
};
