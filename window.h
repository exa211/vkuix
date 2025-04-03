#pragma once
#include "vulkan/vulkan.h"

#define GLFW_NO_INCLUDE
#include "glfw/glfw3.h"

#include <memory>
#include <string>

#include "common.h"

class Window {
public:

  Window(const char* windowTitle, int width, int height) {
    if(glfwInit()) {
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      windowPtr = std::make_unique<GLFWwindow*>(glfwCreateWindow(width, height, windowTitle, nullptr, nullptr));
    }
  }

  void show() const;

  [[nodiscard]] GLFWwindow* getWindowPtr() const;
  [[nodiscard]] VkExtent2D getExtent() const;

private:
  int width;
  int height;

  uptr<GLFWwindow*> windowPtr;
};