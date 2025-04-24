#include <iostream>
#include <vector>

#include "buffer.h"
#include "vkuix.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/transform.hpp>

int main() {

  const sptr<VKUIX::Window> window = VKUIX::createWindow("VKUIX", VKUIX::Dim{1050, 600});
  const sptr<VKUIX::Instance> instance = VKUIX::createInstance(window);

  window->show();

  while (!glfwWindowShouldClose(window->getWindowPtr())) {
    glfwPollEvents();

    const uptr<VKUIX::RenderList>& renderList = VKUIX::getRenderList(instance);
    renderList->roundedRect(25, 25, 200, 50, 5.0f, 4);
    renderList->rect(250, 25, 250, 30);
    renderList->roundedRect(550, 25, 200, 50, 25.0f, 50);

    VKUIX::render(instance, window);
  }
}