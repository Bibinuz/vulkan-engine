#pragma once

#include <GLFW/glfw3.h> 
#include <glm/vec2.hpp>
#include <vulkan/vulkan.hpp>
#include <memory>
#include <span>

namespace lvk::glfw {
struct Deleter {
  void operator()(GLFWwindow* window) const noexcept;
};

using Window = std::unique_ptr<GLFWwindow, Deleter>;
[[nodiscard]] auto create_window(glm::ivec2 size, char const* title) -> Window;

} // End namespace lvk::glfw
