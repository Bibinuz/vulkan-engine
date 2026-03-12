#pragma once

#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <cstdint>

constexpr auto vk_version_v = VK_MAKE_VERSION(1, 3, 0);

namespace lvk {

  struct Gpu {
  vk::PhysicalDevice device{};
  vk::PhysicalDeviceProperties propierties{};
  vk::PhysicalDeviceFeatures features{};
  std::uint32_t queue_family{};
};

[[nodiscard]] auto get_suitable_gpu(vk::Instance instance, vk::SurfaceKHR surface) -> Gpu;

} // End namespace lvk

