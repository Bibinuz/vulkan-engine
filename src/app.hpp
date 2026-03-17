#pragma once
#include "vulkan/vulkan_handles.hpp"
#include "window.hpp"
#include "gpu.hpp"
#include "scopedwaiterdeleter.hpp"
#include "swapchain.hpp"
#include "resource_buffering.hpp"

namespace lvk {
class App {
  public:
    void run();
  
  private:  
    struct RenderSync {
      vk::UniqueSemaphore draw{};
      vk::UniqueFence drawn{};
      vk::CommandBuffer command_buffer{};
    };

    glfw::Window m_window{};
    vk::UniqueInstance m_instance{};
    vk::UniqueSurfaceKHR m_surface{};
    Gpu m_gpu{};
    vk::UniqueDevice m_device{}; 
    vk::Queue m_queue;
    std::optional<Swapchain> m_swapchain{};
    vk::UniqueCommandPool m_render_cmd_pool{};
    Buffered<RenderSync> m_render_sync{};
    std::size_t m_frame_index{};

    //Last member
    ScopedWaiter m_waiter{};
    
    void create_window();
    void create_instance();
    void create_surface();
    void select_gpu();
    void create_device(); 
    void create_swapchain();
    void create_render_sync();
    void main_loop();


};
}
