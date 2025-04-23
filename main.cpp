#include <iostream>
#include <vector>

#include "buffer.h"
#include "vkuix.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/transform.hpp>

int main() {
  sptr<VKUIX::Window> window = VKUIX::createWindow("VKUIX", VKUIX::Dim{1050, 600});
  sptr<VKUIX::Instance> instance = VKUIX::createInstance(window);

  // VKUIX::RenderList list = VKUIX::beginRender(instance);
    //list.renderRectSolid(0, 0, 100, 25, VKUIX::Color());
  // VKUIX::endRender(instance);

  u32 frameIndex{0};

  Buffers::Buffer vertexBuffer{};
  std::vector<VkBackend::Vertex> vertices = {
      {{10.0f, 10.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
      {{100.0f + 10.0f, 0 + 10.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
      {{100.0f + 10.0f, 100.0f + 10.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},

      // {{0, 0}, {0.0f, 0.0f, 0.0f, 1.0f}},
      // {{100.0f, 100.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
      // {{0.0f, 100.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
  };
  createBuffer<VkBackend::Vertex>(vertices, instance->backend.allocator, vertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  mapBuffer<VkBackend::Vertex>(vertices, vertexBuffer, instance->backend.allocator);

  VkViewport viewport{};
  viewport.width = static_cast<float>(window->getExtent().width);
  viewport.height = static_cast<float>(window->getExtent().height);
  viewport.maxDepth = 1.0f;
  viewport.minDepth = 0.0f;
  viewport.x = 0;
  viewport.y = 0;

  VkRect2D scissor = window->getRenderArea();

  Image offscreenImage{};
  offscreenImage.sampleCount = VK_SAMPLE_COUNT_4_BIT;
  offscreenImage.extent = window->getExtent();
  offscreenImage.format = VK_FORMAT_R8G8B8A8_SRGB;
  VkBackend::createImage(instance->backend, offscreenImage);

  window->show();

  while (!glfwWindowShouldClose(window->getWindowPtr())) {
    glfwPollEvents();

    VkBackend::DefaultPushConstant pushConstant{};
    pushConstant.proj = glm::ortho(0.0f, viewport.width, 0.0f, viewport.height, -1.0f, 1.0f);
    pushConstant.model = glm::mat4(1.0f);
    // pushConstant.model = glm::translate(pushConstant.model, glm::vec3{10.0f, 0.0f, 0.0f});

    VkFence &renderFence = instance->renderFrames[frameIndex].renderFence;
    VkSemaphore &renderSema = instance->renderFrames[frameIndex].renderSema;
    VkSemaphore &presentSema = instance->renderFrames[frameIndex].presentSema;
    VkCommandBuffer &cmdBuffer = instance->renderFrames[frameIndex].commandBuffer;
    VkImageView &swapchainImageView = instance->backend.swapchain.images[frameIndex].view;

    vkWaitForFences(instance->backend.device, 1, &renderFence, VK_TRUE, UINT64_MAX);
    u32 swapchainImageIndex;
    vkAcquireNextImageKHR(instance->backend.device, instance->backend.swapchain.swapchain, UINT64_MAX, presentSema, nullptr, &swapchainImageIndex);
    vkResetFences(instance->backend.device, 1, &renderFence);

    VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colorAttachment.imageView = offscreenImage.view;
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

    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    vkCmdBeginRendering(cmdBuffer, &renderInfo);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, instance->defaultPipeline);

    constexpr VkDeviceSize offset = 0;

    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.buffer, &offset);
    vkCmdPushConstants(cmdBuffer, instance->defaultPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkBackend::DefaultPushConstant), &pushConstant);
    vkCmdDraw(cmdBuffer, vertices.size(), 1, 0, 0);

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

    frameIndex = (frameIndex + 1) % 2; // Advance frame index.
  }
}