#include "window.h"

void Window::show() const {
    glfwShowWindow(*windowPtr);
}

GLFWwindow *Window::getWindowPtr() const {
  return *windowPtr;
}
VkExtent2D Window::getExtent() const {
  return VkExtent2D{static_cast<u32>(width), static_cast<u32>(height)};
}