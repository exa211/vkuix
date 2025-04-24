#include "vkuix.h"

int main() {

  const sptr<VKUIX::Window> window = VKUIX::createWindow("VKUIX", VKUIX::Dim{1050, 600});
  const sptr<VKUIX::Instance> instance = VKUIX::createInstance(window);

  window->show();

  while (!glfwWindowShouldClose(window->getWindowPtr())) {
    glfwPollEvents();

    const uptr<VKUIX::RenderList> &renderList = VKUIX::getRenderList(instance);
    renderList->roundRect(10, 10, window->getExtent().width - 20, 50, {5}, 2, VKUIX::Color(41, 41, 43, 255));
    renderList->roundRect(10, 70, 250, 450, {5}, 2, VKUIX::Color(41, 41, 41, 255));

    VKUIX::render(instance, window);
  }
}