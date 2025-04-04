#include <iostream>
#include <vector>

#include "vulkan_backend.h"
#include "window.h"

int main() {

  // Setup the glfw window.
  const Window w{"vkuix", 1050, 600};

  // Get all required extensions from glfw and add our own.
  u32 EXT_COUNT = 0;
  const char **GLFW_EXT = glfwGetRequiredInstanceExtensions(&EXT_COUNT);
  std::vector EXTENSIONS(GLFW_EXT, GLFW_EXT + EXT_COUNT);
  EXTENSIONS.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  VkBackend::Instance engine{}; // Holds all VkObjects for rendering
  VkBackend::setupInstance(engine, EXTENSIONS); // Setup VkInstance
  VkBackend::setupSurface(engine, w.getWindowPtr()); // Setup Window VkSurfaceKHR
  VkBackend::setupDevices(engine); // Choose best suitable physical rendering device
  VkBackend::setupVMA(engine);
  VkBackend::setupSwapchain(w, engine, false);

  VkCommandPool cmdPool{};
  VkBackend::createCommandpool(engine, cmdPool);
  VkCommandBuffer cmdBuffer{};
  VkBackend::createCommandbuffer(engine, cmdPool, cmdBuffer);

  w.show();

  while (!glfwWindowShouldClose(w.getWindowPtr())) {
    glfwPollEvents();
  }
}