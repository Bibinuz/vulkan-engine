#include "window.hpp"
#include <GLFW/glfw3.h>

namespace lvk{

void glfw::Deleter::operator()(GLFWwindow* window) const noexcept {
  glfwDestroyWindow(window);
  glfwTerminate();
}


} // End namespace lvk
