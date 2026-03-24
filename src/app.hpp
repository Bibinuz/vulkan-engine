#pragma once

#include "commandblock.hpp"
#include "dearimgui.hpp"
#include "descriptorbuffer.hpp"
#include "gpu.hpp"
#include "resource_buffering.hpp"
#include "scopedwaiterdeleter.hpp"
#include "shaderprogram.hpp"
#include "swapchain.hpp"
#include "texture.hpp"
#include "transform.hpp"
#include "vma.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "window.hpp"
#include <bit>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <vector>

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

    [[nodiscard]] auto asset_path(std::string_view uri) const -> std::filesystem::path;

    void create_window();
    void create_instance();
    void create_surface();
    void select_gpu();
    void create_device();
    void create_swapchain();
    void create_render_sync();
    void create_imgui();
    void create_allocator();
    void create_shader();
    void create_shader_resources();
    void create_cmd_block_pool();
    auto create_command_block() const -> CommandBlock;
    void create_descriptor_pool();
    void create_pipeline_layout();
    void create_descriptor_sets();

    auto acquire_render_target() -> bool;
    auto allocate_sets() const -> std::vector<vk::DescriptorSet>;
    auto begin_frame() -> vk::CommandBuffer;
    void bind_descriptor_sets(vk::CommandBuffer const command_buffer) const;
    void update_view();
    void update_instances();
    void transition_for_render(vk::CommandBuffer command_buffer) const;
    void render(vk::CommandBuffer command_buffer);
    void transition_for_present(vk::CommandBuffer command_buffer) const;
    void submit_and_present();
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
    vma::Allocator m_allocator{}; // anywhere between device and shader.
    std::optional<ShaderProgram> m_shader{};
    vma::Buffer m_vbo{};
    vk::UniqueCommandPool m_cmd_block_pool{};
    vk::UniqueDescriptorPool m_descriptor_pool{};
    std::vector<vk::UniqueDescriptorSetLayout> m_set_layouts{};
    std::vector<vk::DescriptorSetLayout> m_set_layout_views{};
    vk::UniquePipelineLayout m_pipeline_layout{};
    Buffered<std::vector<vk::DescriptorSet>> m_descriptor_sets{};
    std::optional<DescriptorBuffer> m_view_ubo{};
    std::optional<Texture> m_texture{};
    Transform m_view_transform{};
    std::vector<glm::mat4> m_instance_data{};
    std::array<Transform, 2> m_instances{};
    std::optional<DescriptorBuffer> m_instance_ssbo{};
    bool m_wireframe{};

    // Last member, first to be deleted
    ScopedWaiter m_waiter{};
};

template <typename T> [[nodiscard]] constexpr auto to_byte_array(T const &t) {
    return std::bit_cast<std::array<std::byte, sizeof(T)>>(t);
}

} // namespace lvk
