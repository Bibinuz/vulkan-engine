#pragma once
#include "shaderprogram.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "window.hpp"
#include "gpu.hpp"
#include "scopedwaiterdeleter.hpp"
#include "swapchain.hpp"
#include "resource_buffering.hpp"
#include "dearimgui.hpp"
#include <filesystem>

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
    
    void create_window();
    void create_instance();
    void create_surface();
    void select_gpu();
    void create_device(); 
    void create_swapchain();
    void create_render_sync();
    void create_imgui();
    void create_shader();

    auto acquire_render_target() -> bool;
    auto begin_frame() -> vk::CommandBuffer;
    void transition_for_render(vk::CommandBuffer command_buffer) const;
    void render(vk::CommandBuffer command_buffer);
    void transition_for_present(vk::CommandBuffer command_buffer) const;
    void submit_and_present();
    [[nodiscard]] auto asset_path(std::string_view uri) const -> std::filesystem::path;
    void inspect();
    void draw(vk::CommandBuffer command_buffer) const;
    void main_loop();

    glfw::Window m_window{};
    vk::UniqueInstance m_instance{};
    vk::UniqueSurfaceKHR m_surface{};
    Gpu m_gpu{};
    vk::UniqueDevice m_device{}; 
    vk::Queue m_queue{};
    std::optional<Swapchain> m_swapchain{};
    vk::UniqueCommandPool m_render_cmd_pool{};
    Buffered<RenderSync> m_render_sync{};
    std::size_t m_frame_index{};
    glm::ivec2 m_framebuffer_size{};
    std::optional<RenderTarget> m_render_target{};
    std::optional<DearImGui> m_imgui{};
    std::filesystem::path m_assets_dir{}; 
    std::optional<ShaderProgram> m_shader{};
    //Last member, first to be deleted
    ScopedWaiter m_waiter{};
};
}
