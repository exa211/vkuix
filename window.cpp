#include "window.h"

Window::Window(const char *windowTitle, const int width, const int height) : width(width), height(height){
  if(glfwInit()) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    windowPtr = std::make_unique<GLFWwindow*>(glfwCreateWindow(width, height, windowTitle, nullptr, nullptr));
  }
}


void Window::show() const {
  glfwShowWindow(*windowPtr);
}

GLFWwindow *Window::getWindowPtr() const {
  return *windowPtr;
}
VkExtent2D Window::getExtent() const {
  return {static_cast<u32>(width), static_cast<u32>(height)};
}