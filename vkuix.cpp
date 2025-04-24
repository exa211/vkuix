#include "vkuix.h"

#include <glm/ext/matrix_clip_space.hpp>

#include "buffer.h"

sptr<VKUIX::Window> VKUIX::createWindow(const char *title, Dim dimension) {
  return std::make_shared<VKUIX::Window>(title, dimension.width, dimension.height);
}

sptr<VKUIX::Instance> VKUIX::createInstance(const sptr<Window> &window, const std::vector<const char *> *additionalExtensions) {

  sptr<VKUIX::Instance> instance = std::make_shared<VKUIX::Instance>();

  // Instance extensions
  std::vector<const char *> EXTENSIONS = VkBackend::getDefaultInstanceExt();
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

  // Viewport
  instance->viewport.width = static_cast<float>(window->getExtent().width);
  instance->viewport.height = static_cast<float>(window->getExtent().height);
  instance->viewport.maxDepth = 1.0f;
  instance->viewport.minDepth = 0.0f;
  instance->viewport.x = 0;
  instance->viewport.y = 0;

  // Offscreen MSAA Antialiasing image
  instance->msaaImage.sampleCount = VkBackend::ANTI_ALIASING_COUNT;
  instance->msaaImage.extent = window->getExtent();
  instance->msaaImage.format = VkBackend::COLOR_FORMAT;
  VkBackend::createImage(instance->backend, instance->msaaImage);

  instance->renderList = std::make_unique<RenderList>();
  return instance;
}

uptr<VKUIX::RenderList> &VKUIX::getRenderList(const sptr<Instance> &instance) {
  return instance->renderList;
}

void VKUIX::render(const sptr<Instance> &instance, const sptr<Window>& window) {

  Buffers::Buffer vertexBuffer{};
  Buffers::createBuffer(instance->renderList->getVertices(), instance->backend.allocator, vertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  Buffers::mapBuffer(instance->renderList->getVertices(), vertexBuffer, instance->backend.allocator);

  VkBackend::DefaultPushConstant pushConstant{};
  pushConstant.proj = glm::ortho(0.0f, instance->viewport.width, 0.0f, instance->viewport.height, -1.0f, 1.0f);
  pushConstant.model = glm::mat4(1.0f);
  // pushConstant.model = glm::translate(pushConstant.model, glm::vec3{10.0f, 0.0f, 0.0f});

  VkFence &renderFence = instance->renderFrames[instance->frameIndex].renderFence;
  VkSemaphore &renderSema = instance->renderFrames[instance->frameIndex].renderSema;
  VkSemaphore &presentSema = instance->renderFrames[instance->frameIndex].presentSema;
  VkCommandBuffer &cmdBuffer = instance->renderFrames[instance->frameIndex].commandBuffer;
  VkImageView &swapchainImageView = instance->backend.swapchain.images[instance->frameIndex].view;

  vkWaitForFences(instance->backend.device, 1, &renderFence, VK_TRUE, UINT64_MAX);
  u32 swapchainImageIndex;
  vkAcquireNextImageKHR(instance->backend.device, instance->backend.swapchain.swapchain, UINT64_MAX, presentSema, nullptr, &swapchainImageIndex);
  vkResetFences(instance->backend.device, 1, &renderFence);

  VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
  colorAttachment.imageView = instance->msaaImage.view;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue = {{0.1f, 0.1f, 0.1f, 1.0f}};
  colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
  colorAttachment.resolveImageView = swapchainImageView;
  colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

  VkRenderingInfo renderInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
  renderInfo.renderArea = window->getRenderArea();
  renderInfo.layerCount = 1;
  renderInfo.colorAttachmentCount = 1;
  renderInfo.pColorAttachments = &colorAttachment;

  VkCommandBufferBeginInfo cmdBegin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkResetCommandBuffer(cmdBuffer, 0);
  vkBeginCommandBuffer(cmdBuffer, &cmdBegin);

  VkBackend::transitionImage(cmdBuffer, instance->backend.swapchain.images[swapchainImageIndex].vkImage,
                             VK_PIPELINE_STAGE_2_NONE, 0,
                             VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                             VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  VkRect2D scissor = window->getRenderArea();

  vkCmdSetViewport(cmdBuffer, 0, 1, &instance->viewport);
  vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

  vkCmdBeginRendering(cmdBuffer, &renderInfo);

  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, instance->defaultPipeline);

  constexpr VkDeviceSize offset = 0;

  vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.buffer, &offset);
  vkCmdPushConstants(cmdBuffer, instance->defaultPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkBackend::DefaultPushConstant), &pushConstant);
  vkCmdDraw(cmdBuffer, instance->renderList->getVertices().size(), 1, 0, 0);

  vkCmdEndRendering(cmdBuffer);

  VkBackend::transitionImage(cmdBuffer, instance->backend.swapchain.images[swapchainImageIndex].vkImage,
                             VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                             VK_PIPELINE_STAGE_2_NONE, 0,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  vkEndCommandBuffer(cmdBuffer);

  std::vector commandBuffers = {cmdBuffer};
  const std::vector<VkPipelineStageFlags> dstMasks = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = commandBuffers.size();
  submitInfo.pCommandBuffers = commandBuffers.data();

  submitInfo.pWaitSemaphores = &presentSema;
  submitInfo.pSignalSemaphores = &renderSema;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pWaitDstStageMask = dstMasks.data();

  if (vkQueueSubmit(instance->backend.graphicsQueue, 1, &submitInfo, renderFence) != VK_SUCCESS)
    LOG(W, "Could not submit queue.");

  VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &instance->backend.swapchain.swapchain;
  presentInfo.pImageIndices = &swapchainImageIndex;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &renderSema;

  vkQueuePresentKHR(instance->backend.graphicsQueue, &presentInfo);
  instance->frameIndex = (instance->frameIndex + 1) % instance->backend.swapchain.framebufferingAmount; // Advance frame index.

  instance->renderList->clear();

}
