#include "mainheader.h"

using namespace std;

void LineShader::init(ShadedPathEngine& engine, ShaderState &shaderState)
{
	ShaderBase::init(engine);
	resources.setResourceDefinition(&vulkanResourceDefinition);

	// create shader modules
	vertShaderModule = resources.createShaderModule("line.vert.spv");
	fragShaderModule = resources.createShaderModule("line.frag.spv");

	// descriptor set layout
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool);

	globalLineSubShader.init(this, "GlobalLineSubshader");
}

void LineShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
}

void LineShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
}


void LineShader::initialUpload()
{
}

void LineShader::createDescriptorSetLayout()
{
	Error("remove this method from base class!");
}

void LineShader::createDescriptorSets(ThreadResources& tr)
{
	Error("remove this method from base class!");
}

void LineShader::createCommandBuffer(ThreadResources& tr)
{
}

void LineShader::addCurrentCommandBuffer(ThreadResources& tr) {
};

void LineShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye)
{
}

void LineShader::clearLocalLines(ThreadResources& tr)
{
}

void LineShader::addGlobalConst(vector<LineDef>& linesToAdd)
{
}

void LineShader::addLocalLines(std::vector<LineDef>& linesToAdd, ThreadResources& tr)
{
}

void LineShader::uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	if (!enabled) return;
}

void LineShader::update(ShaderUpdateElement *el)
{
	LineShaderUpdateElement *u = static_cast<LineShaderUpdateElement*>(el);
	// TODO think about moving update_finished logic to base class
	Log("update line shader global buffer via slot " << u->arrayIndex << " update num " << u->num << endl);
	Log("  --> push " << u->linesToAdd->size() << " lines to GPU" << endl);
	GlobalResourceSet set = getInactiveResourceSet();
	updateAndSwitch(u->linesToAdd, set);
	Log("update line shader global end " << u->arrayIndex << " update num " << u->num << endl);
}

void LineShader::updateGlobal(std::vector<LineDef>& linesToAdd)
{
	//Log("LineShader update global start");
	//engine->printUpdateArray(updateArray);
	int i = (int)engine->reserveUpdateSlot(updateArray);
	updateArray[i].shaderInstance = this; // TODO move elsewhere - to shader init?
	updateArray[i].arrayIndex = i; // TODO move elsewhere - to shader init?
	updateArray[i].linesToAdd = &linesToAdd;
	//engine->shaderUpdateQueue.push(i);
	engine->pushUpdate(&updateArray[i]);
	//Log("LineShader update end         ");
	//engine->printUpdateArray(updateArray);
}

void LineShader::updateAndSwitch(std::vector<LineDef>* linesToAdd, GlobalResourceSet set)
{
	if (set == GlobalResourceSet::SET_A) {
	}
	else Error("not implemented");
}

void LineShader::resourceSwitch(GlobalResourceSet set)
{
	if (set == GlobalResourceSet::SET_A) {
		Log("LineShader::resourceSwitch() to SET_A" << endl;)
	}
	else Error("not implemented");
}

void LineShader::handleUpdatedResources(ThreadResources& tr)
{
}

void LineShader::switchGlobalThreadResources(ThreadResources& res)
{
}

LineShader::~LineShader()
{
	Log("LineShader destructor\n");
	if (!enabled) {
		return;
	}
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void LineShader::destroyThreadResources(ThreadResources& tr)
{
	auto& trl = tr.lineResources;
	vkDestroyFramebuffer(device, trl.framebuffer, nullptr);
	vkDestroyFramebuffer(device, trl.framebufferAdd, nullptr);
	vkDestroyRenderPass(device, trl.renderPass, nullptr);
	vkDestroyRenderPass(device, trl.renderPassAdd, nullptr);
	vkDestroyPipeline(device, trl.graphicsPipeline, nullptr);
	vkDestroyPipeline(device, trl.graphicsPipelineAdd, nullptr);
	vkDestroyPipelineLayout(device, trl.pipelineLayout, nullptr);
	vkDestroyBuffer(device, trl.uniformBuffer, nullptr);
	vkFreeMemory(device, trl.uniformBufferMemory, nullptr);
	vkDestroyBuffer(device, trl.vertexBufferAdd, nullptr);
	vkFreeMemory(device, trl.vertexBufferAddMemory, nullptr);
	if (engine->isStereo()) {
		vkDestroyFramebuffer(device, trl.framebuffer2, nullptr);
		vkDestroyFramebuffer(device, trl.framebufferAdd2, nullptr);
		vkDestroyBuffer(device, trl.uniformBuffer2, nullptr);
		vkFreeMemory(device, trl.uniformBufferMemory2, nullptr);
	}
}
