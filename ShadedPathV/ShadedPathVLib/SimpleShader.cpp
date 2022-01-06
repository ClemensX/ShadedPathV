#include "pch.h"

void SimpleShader::init(ShadedPathEngine &engine)
{
    this->device = engine.global.device;
    this->global = &engine.global;
    this->engine = &engine;
}

void SimpleShader::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        Error("failed to create descriptor set layout!");
    }
}

void SimpleShader::createUniformBuffer(ThreadResources& res)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    global->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        res.uniformBufferTriangle, res.uniformBufferMemoryTriangle);
}

void SimpleShader::updatePerFrame(ThreadResources& tr)
{
    //Log("time: " << engine->gameTime.getTimeSystemClock() << endl);
    //Log("game time: " << engine->gameTime.getTimeGameClock() << endl);
    //Log("game time rel: " << setprecision(27) << engine->gameTime.getTime() << endl);
    Log("time delta: " << setprecision(27) << engine->gameTime.getTimeDelta() << endl);
}

SimpleShader::~SimpleShader()
{
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}