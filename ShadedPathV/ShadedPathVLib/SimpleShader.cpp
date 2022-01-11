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

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

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

void SimpleShader::createDescriptorSets(ThreadResources& res)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = res.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &res.descriptorSetTriangle) != VK_SUCCESS) {
        Error("failed to allocate descriptor sets!");
    }

    // populate descriptor set:
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = res.uniformBufferTriangle;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->imageView;
    imageInfo.sampler = global->textureSampler;

    array<VkWriteDescriptorSet, 2> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = res.descriptorSetTriangle;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = res.descriptorSetTriangle;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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
    ubo.model = glm::rotate(glm::mat4(1.0f), (float)(seconds * glm::radians(90.0f)), glm::vec3(0.0f, 0.0f, 1.0f));
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

void SimpleShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, VkBuffer indexBuffer)
{
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tr.graphicsPipelineTriangle);
    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    // bind descriptor sets:
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tr.pipelineLayoutTriangle, 0, 1, &tr.descriptorSetTriangle, 0, nullptr);

    //vkCmdDraw(commandBuffer, static_cast<uint32_t>(simpleShader.vertices.size()), 1, 0, 0);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

SimpleShader::~SimpleShader()
{
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}