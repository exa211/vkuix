#pragma once

#include <glm/glm.hpp>

#include "vma/vk_mem_alloc.h"

#include "common.h"
#include "image_types.h"
#include "shader.h"
#include "window.h"

namespace VkBackend {

  inline constexpr bool VALIDATION = false;
  inline constexpr VkSampleCountFlagBits ANTI_ALIASING_COUNT = VK_SAMPLE_COUNT_8_BIT;

  const std::vector<const char*> VAL_LAYERS = {
      "VK_LAYER_KHRONOS_validation"}; // This val layer leaks memory, cant get messages to show up that would justify

  const std::vector DEVICE_EXT = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
      VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME};

  inline std::vector<const char *> getDefaultInstanceExt() {
    u32 EXT_COUNT = 0;
    const char **GLFW_EXT = glfwGetRequiredInstanceExtensions(&EXT_COUNT);
    std::vector EXTENSIONS(GLFW_EXT, GLFW_EXT + EXT_COUNT);
    EXTENSIONS.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return EXTENSIONS;
  }

  // Error callback
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT mesSever,
      VkDebugUtilsMessageTypeFlagsEXT mesType,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
      void *pUserData) {
    std::cout << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }

  struct QueueFamilyInfo {
    std::optional<u32> presentFamily;
    std::optional<u32> graphicsFamily;
  };

  struct Swapchain {
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;

    u32 framebufferingAmount{0};

    std::vector<Image> images;
  };

  struct Instance {
    VkInstance vkInstance{};
    VkSurfaceKHR surface{};
    VkPhysicalDevice physDevice{};
    VkDevice device{};

    QueueFamilyInfo queueFamilies;
    VkQueue graphicsQueue{};

    VmaAllocator allocator{};

    Swapchain swapchain{};

    std::unordered_map<const char*, VkPipeline> pipelineRepository{};
  };

  struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
  };

  struct Vertex {
    glm::vec2 pos;
    glm::vec4 col;
    u32 tex_id = 0;

    static VertexInputDescription getVertexDescription() {
      VertexInputDescription description{};
      description.bindings.reserve(1);

      // Binding | 0
      VkVertexInputBindingDescription binding0{};
      binding0.binding = 0;
      binding0.stride = sizeof(Vertex);
      binding0.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      description.bindings.push_back(binding0);

      // Vertex Position Attribute | Loc: 0
      VkVertexInputAttributeDescription posAttrib{};
      posAttrib.binding = 0;
      posAttrib.location = 0;
      posAttrib.format = VK_FORMAT_R32G32_SFLOAT;
      posAttrib.offset = offsetof(Vertex, pos);

      // Vertex Color Attribute | Loc: 1
      VkVertexInputAttributeDescription colAttrib{};
      colAttrib.binding = 0;
      colAttrib.location = 1;
      colAttrib.format = VK_FORMAT_R32G32B32A32_SFLOAT;
      colAttrib.offset = offsetof(Vertex, col);

      description.attributes.push_back(posAttrib);
      description.attributes.push_back(colAttrib);

      return description;
    }

  };

  struct DefaultPushConstant {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
  };

  void setupInstance(Instance &instance, const std::vector<const char *>& extensions);
  void setupSurface(Instance &instance, GLFWwindow *pWin);
  void setupDevices(Instance &instance);
  void setupVMA(Instance &instance);
  void setupQueues(Instance &instance);
  void setupSwapchain(const sptr<VKUIX::Window> &window, Instance &instance, bool resize = false);

  // Command methods
  void createCommandpool(Instance &instance, VkCommandPool &pool, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  void createCommandbuffer(const Instance &instance, const VkCommandPool &pool, VkCommandBuffer &buffer);

  // Sync object methods
  struct RenderFrame {
    VkCommandBuffer commandBuffer;
    VkFence renderFence;
    VkSemaphore renderSema;
    VkSemaphore presentSema;
  };
  void createFence(const Instance &instance, VkFence &fenceOut);
  void createSemaphore(const Instance &instance, VkSemaphore &semaOut);

  // Image methods
  void createImage(const Instance &instance, Image &image);

  void transitionImage(VkCommandBuffer &cmdBuffer, VkImage &image,
    VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccessMask,
    VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccessMask,
    VkImageLayout oldLayout, VkImageLayout newLayout);

  // Descriptor methods
  struct DescriptorPoolInfo {
    std::vector<VkDescriptorPoolSize> sizes;
    VkDescriptorPoolCreateFlagBits flags;
    u32 maxSets;
  };
  void createDescriptorPool(const Instance &instance, const DescriptorPoolInfo &poolInfo, VkDescriptorPool &poolOut);

  struct DescriptorSetLayoutInfo {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    VkDescriptorSetLayoutCreateFlags createFlags;
    const void *pExt = nullptr;
  };
  void createDescriptorLayout(const Instance &instance, const DescriptorSetLayoutInfo &layoutInfo, VkDescriptorSetLayout &descLayoutOut);

  struct DescriptorSetAllocInfo {
    VkDescriptorPool *pPool;
    std::vector<VkDescriptorSetLayout> layouts;
    u16 setCount = 1;
    const void *pExt = nullptr;
  };
  void allocDescriptorSets(const Instance &instance, const DescriptorSetAllocInfo &allocInfo, VkDescriptorSet &descSetOut);

  // Pipeline Methods
  void createDynamicGraphicsPipeline(const Instance &instance, Shader &shader,
    std::vector<VkDescriptorSetLayout> &layouts, VkPipeline &dynamicPipeline, VkPipelineLayout &pipelineLayout);

  // Future compat wip - for devices that dont support dynamic rendering.
  struct RenderpassInfo {
    std::vector<VkAttachmentDescription> colorDescriptions;
    std::vector<VkAttachmentDescription> depthDescriptions;
    std::vector<VkSubpassDescription> subpassDescriptions;
    std::vector<VkSubpassDependency> subpassDependencies;
  };
  void createRenderpass(const Instance &instance, RenderpassInfo &rpInfo, VkRenderPass &renderpass);

} // namespace VkBackend

namespace VKUIX {
}