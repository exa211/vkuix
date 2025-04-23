#pragma once

#include "vulkan_backend.h"

namespace VKUIX {

  struct Dim {
    u32 width;
    u32 height;
  };

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

    std::vector<VkBackend::RenderFrame> renderFrames{};
  };

  struct RenderList {
    std::vector<VkBackend::Vertex> verticesBatched{};
  };

  sptr<Window> createWindow(const char* title, Dim dimension);
  sptr<Instance> createInstance(const sptr<Window>& window, const std::vector<const char*>* additionalExtensions = nullptr);

  RenderList beginRender(const sptr<Instance> &instance);

  // Geometry drawing methods
  void drawRect();

}