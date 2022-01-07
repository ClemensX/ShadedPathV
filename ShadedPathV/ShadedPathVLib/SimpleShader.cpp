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
    //Log("time delta: " << setprecision(27) << engine->gameTime.getTimeDelta() << endl);
    //Log("time rel in s: " << setprecision(27) << engine->gameTime.getTimeSeconds() << endl);
    double seconds = engine->gameTime.getTimeSeconds();
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(), (float)(seconds * glm::radians(90.0f)), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), engine->getAspect(), 0.1f, 10.0f);
    // flip y:
    ubo.proj[1][1] *= -1;

    // copy ubo to GPU:
    void* data;
    vkMapMemory(device, tr.uniformBufferMemoryTriangle, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, tr.uniformBufferMemoryTriangle);
}

SimpleShader::~SimpleShader()
{
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}