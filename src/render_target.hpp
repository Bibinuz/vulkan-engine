#pragma once
#include "vulkan/vulkan_handles.hpp"

namespace lvk {
  struct RenderTarget {
    vk::Image image{};
    vk::ImageView image_view{};
    vk::Extent2D extent{};
  };
}
