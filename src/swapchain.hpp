#pragma once

#include "gpu.hpp"
#include "render_target.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <cstddef>
#include <optional>

namespace lvk {
  class Swapchain {
    public:
      explicit Swapchain(vk::Device device, Gpu const& gpu, vk::SurfaceKHR surface, glm::ivec2 size);
      auto recreate(glm::ivec2 size) -> bool;

      [[nodiscard]] auto get_size() const -> glm::ivec2 {
        return {m_ci.imageExtent.width, m_ci.imageExtent.height};
      }

      [[nodiscard]] auto get_present_semaphore() const -> vk::Semaphore;
      [[nodiscard]] auto acquire_next_image(vk::Semaphore const to_signal) -> std::optional<RenderTarget>;
      [[nodiscard]] auto present(vk::Queue const queue) -> bool;
      [[nodiscard]] auto base_barrier() const -> vk::ImageMemoryBarrier2;

    private:
      void populate_images();
      void create_image_views();
      void create_present_semaphores();
      

      vk::Device m_device{};
      Gpu m_gpu{};
      vk::SwapchainCreateInfoKHR m_ci{};
      vk::UniqueSwapchainKHR m_swapchain{};
      std::vector<vk::Image> m_images{};
      std::vector<vk::UniqueImageView> m_image_views{};
      std::vector<vk::UniqueSemaphore> m_present_semaphores{};
      std::optional<std::size_t> m_image_index{};
  };

}
