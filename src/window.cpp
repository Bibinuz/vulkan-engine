#include "window.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <print>
#include <stdexcept>

namespace lvk{

void glfw::Deleter::operator()(GLFWwindow* window) const noexcept {
  glfwDestroyWindow(window);
  glfwTerminate();
}

auto glfw::create_window(glm::ivec2 size, const char *title) -> Window {
  static auto const on_error = [](int const code, char const* description) {
    std::println(stderr, "[GLFW] Error {}: {}", code, description);
  };
  glfwSetErrorCallback(on_error);

  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error{"Failed to initialize GLFW"};
  }
  if (glfwVulkanSupported() != GLFW_TRUE) {
    throw std::runtime_error{"Vulkan not supported"};
  }
  auto ret = Window{};

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  ret.reset(glfwCreateWindow(size.x, size.y, title, nullptr, nullptr));
  if (!ret) {
    throw std::runtime_error{"Faled to create GLFW Window"};
  }
  return ret;
}

auto glfw::instance_extensions() -> std::span<char const* const> {
  auto count = std::uint32_t{};
  auto const* extensions = glfwGetRequiredInstanceExtensions(&count);
  return {extensions, static_cast<std::size_t>(count)};
}

auto glfw::create_surface(GLFWwindow *window, const vk::Instance instance) -> vk::UniqueSurfaceKHR {
  VkSurfaceKHR ret{};
  auto const result = glfwCreateWindowSurface(instance, window, nullptr, &ret);
  if (result != VK_SUCCESS || ret == VkSurfaceKHR{}) {
    throw std::runtime_error{"Failed to create Vulkan Surface"};
  }
  return vk::UniqueSurfaceKHR{ret, instance};
}

auto glfw::framebuffer_size(GLFWwindow *window) -> glm::ivec2 {
  auto ret = glm::ivec2{};
  glfwGetFramebufferSize(window, &ret.x, &ret.y);
  return ret;
}

} // End namespace lvk
