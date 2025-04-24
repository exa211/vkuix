#include "window.h"

// _WIN32
#ifdef _WIN32
  #define GLFW_EXPOSE_NATIVE_WIN32
  #include "glfw3native.h"
  #include <windows.h>
  #include <dwmapi.h>
  #pragma comment(lib, "Dwmapi.lib")
#endif

VKUIX::Window::Window(const char *windowTitle, const int width, const int height) : width(width), height(height){
  if(glfwInit()) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    windowPtr = std::make_unique<GLFWwindow*>(glfwCreateWindow(width, height, windowTitle, nullptr, nullptr));

#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(*windowPtr);
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &darkMode, sizeof(darkMode));
#endif

  }
}


void VKUIX::Window::show() const {
  glfwShowWindow(*windowPtr);
}

GLFWwindow *VKUIX::Window::getWindowPtr() const {
  return *windowPtr;
}
VkExtent2D VKUIX::Window::getExtent() const {
  return {static_cast<u32>(width), static_cast<u32>(height)};
}
VkRect2D VKUIX::Window::getRenderArea() const {
  return {0, 0, {getExtent()}};
}