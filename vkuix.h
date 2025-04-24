#pragma once

#include <glm/gtc/constants.hpp>

#include "renderlist.h"

namespace VKUIX {

  struct Instance {
    VkBackend::Instance backend;

    VkCommandPool cmdPool{};
    VkCommandPool uploadPool{};
    VkCommandBuffer cmdBuffer{};

    VkDescriptorPool mainDescPool{};
    VkDescriptorSetLayout descLayoutUniform{};
    VkDescriptorSet mainDescriptor{};

    VkPipeline defaultPipeline{};
    VkPipelineLayout defaultPipelineLayout{};

    VkViewport viewport{};
    Image msaaImage{};

    std::vector<VkBackend::RenderFrame> renderFrames{};
    u32 frameIndex{0};

    uptr<RenderList> renderList{};
  };

  sptr<Window> createWindow(const char *title, Dim dimension);
  sptr<Instance> createInstance(const sptr<Window> &window, const std::vector<const char *> *additionalExtensions = nullptr);

  uptr<RenderList> &getRenderList(const sptr<Instance> &instance);
  void render(const sptr<Instance> &instance, const sptr<Window> &window);

} // namespace VKUIX