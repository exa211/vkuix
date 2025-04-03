#pragma once

#include "vma/vk_mem_alloc.h"

#include "window.h"

#include "common.h"
#include "image_types.h"

namespace VkBackend {

  const std::vector DEVICE_EXT = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_MAINTENANCE1_EXTENSION_NAME};

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

    std::vector<Image> images;
  };

  struct Instance {
    VkInstance vkInstance{};
    VkSurfaceKHR surface{};
    VkPhysicalDevice physDevice{};
    VkDevice device{};
    QueueFamilyInfo queueFamilies;

    VmaAllocator allocator{};

    Swapchain swapchain{};
  };

  void setupInstance(Instance &instance, const std::vector<const char *> &extensions);
  void setupSurface(Instance &instance, GLFWwindow *pWin);
  void setupDevices(Instance &instance);
  void setupVMA(Instance &instance);
  void setupQueues(Instance &instance);
  void setupSwapchain(const Window & window, Instance& instance, bool resize);

} // namespace VkBackend