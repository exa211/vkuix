#pragma once
#include "vulkan/vulkan.h"

#define GLFW_NO_INCLUDE
#include "glfw/glfw3.h"

#include <memory>
#include <string>

#include "common.h"


namespace VKUIX {

  class Window {
  public:

    Window(const char* windowTitle, int width, int height);

    void show() const;

    [[nodiscard]] GLFWwindow* getWindowPtr() const;
    [[nodiscard]] VkExtent2D getExtent() const;
    [[nodiscard]] VkRect2D getRenderArea() const;

  private:
    int width;
    int height;

    uptr<GLFWwindow*> windowPtr;
  };

}