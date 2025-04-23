#include "vkuix.h"

sptr<VKUIX::Window> VKUIX::createWindow(const char *title, Dim dimension) {
  return std::make_shared<VKUIX::Window>(title, dimension.width, dimension.height);
}

sptr<VKUIX::Instance> VKUIX::createInstance(const sptr<Window>& window, const std::vector<const char*>* additionalExtensions) {

  sptr<VKUIX::Instance> instance = std::make_shared<VKUIX::Instance>();

  // Instance extensions
  std::vector<const char*> EXTENSIONS = VkBackend::getDefaultInstanceExt();
  if (additionalExtensions) {
    EXTENSIONS.insert(EXTENSIONS.end(), additionalExtensions->begin(), additionalExtensions->end());
  }

  VkBackend::setupInstance(instance->backend, EXTENSIONS); // Setup VkInstance
  VkBackend::setupSurface(instance->backend, window->getWindowPtr()); // Setup Window VkSurfaceKHR
  VkBackend::setupDevices(instance->backend); // Choose best suitable physical rendering device
  VkBackend::setupVMA(instance->backend);
  VkBackend::setupSwapchain(window, instance->backend, false);

  VkBackend::createCommandpool(instance->backend, instance->cmdPool);
  VkBackend::createCommandpool(instance->backend, instance->uploadPool);

  VkBackend::DescriptorPoolInfo poolInfo{.maxSets = 1};
  poolInfo.sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
  VkBackend::createDescriptorPool(instance->backend, poolInfo, instance->mainDescPool);

  VkBackend::DescriptorSetLayoutInfo layoutInfo{};
  VkBackend::createDescriptorLayout(instance->backend, layoutInfo, instance->descLayoutUniform);

  VkBackend::DescriptorSetAllocInfo allocInfo{};
  allocInfo.pPool = &instance->mainDescPool;
  allocInfo.layouts = {instance->descLayoutUniform};
  VkBackend::allocDescriptorSets(instance->backend, allocInfo, instance->mainDescriptor);

  Shader defaultShader{instance->backend.device, "default"};

  std::vector pipelineLayouts = {instance->descLayoutUniform};
  VkBackend::createDynamicGraphicsPipeline(instance->backend, defaultShader, pipelineLayouts, instance->defaultPipeline, instance->defaultPipelineLayout);

  instance->renderFrames.reserve(instance->backend.swapchain.framebufferingAmount);
  for (int i = 0; i < instance->backend.swapchain.framebufferingAmount; ++i) {
    VkBackend::createCommandbuffer(instance->backend, instance->cmdPool, instance->renderFrames[i].commandBuffer);
    VkBackend::createFence(instance->backend, instance->renderFrames[i].renderFence);
    VkBackend::createSemaphore(instance->backend, instance->renderFrames[i].renderSema);
    VkBackend::createSemaphore(instance->backend, instance->renderFrames[i].presentSema);
  }

  return instance;
}
