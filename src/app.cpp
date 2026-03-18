#include "app.hpp"
#include "shaderprogram.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <print>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <vulkan/vulkan_hpp_macros.hpp>
#include "gpu.hpp"
#include <ranges>
#include <chrono>
#include <imgui.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE


namespace lvk {

namespace fs = std::filesystem;

auto locate_assets_dir() -> fs::path {
  static constexpr std::string_view dir_name_v{"assets"};
  for (auto path = fs::current_path();
      !path.empty() && path.has_parent_path(); path = path.parent_path()) {
    auto ret = path / dir_name_v;
    if (fs::is_directory(ret)) { return ret; }
  }
  std::println("[lvk] Warning: could not locate '{}'' directory", dir_name_v);
  return fs::current_path();
}

[[nodiscard]] auto get_layers(std::span<char const* const> desired) -> std::vector<char const*> {
  auto ret = std::vector<char const*>{};
  ret.reserve(desired.size());
  auto const avaiable = vk::enumerateInstanceLayerProperties();
  for (char const* layer : desired) {
    auto const pred = [layer = std::string_view{layer}]
    (vk::LayerProperties const& propierties)
    {return propierties.layerName == layer; };

    if (std::ranges::find_if(avaiable, pred) == avaiable.end()) {
      std::println("[lvk] [WARNING] Vulkan Layer '{}' not found", layer);
      continue;
    }
    ret.push_back(layer);
  }
  return ret;
}

[[nodiscard]] auto to_spir_v(fs::path const& path) -> std::vector<std::uint32_t> {
  auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
  if (!file.is_open()) {
    throw std::runtime_error{
      std::format("Failed to open file: '{}'", path.generic_string())};
  }

  auto const size = file.tellg();
  auto const usize = static_cast<std::uint64_t>(size);
  if(usize % sizeof(std::uint32_t) != 0) {
    throw std::runtime_error{std::format("Invalid SPIR-V size: '{}'", usize)};
  }
  file.seekg({}, std::ios::beg);
  auto ret = std::vector<std::uint32_t>{};
  ret.resize(usize / sizeof(std::uint32_t));
  void* data = ret.data();
  file.read(static_cast<char*>(data), size);
  return ret;
}

void App::run() {

  m_assets_dir = locate_assets_dir();

  create_window();
  create_instance();
  create_surface();
  select_gpu();  
  create_device();
  create_swapchain();
  create_render_sync();
  create_imgui();
  create_shader();
  main_loop();
}

void App::create_window() {
  m_window = glfw::create_window({1280, 720}, "Learn Vulkan");
}

void App::create_instance() {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();
  auto const loader_version = vk::enumerateInstanceVersion();
  if (loader_version < vk_version_v) {
    throw std::runtime_error{"Loader does not support vulkan 1.3"};
  }
  
  auto app_info = vk::ApplicationInfo{};
  app_info.setPApplicationName("Learn Vulkan").setApiVersion(vk_version_v);

  auto instance_ci = vk::InstanceCreateInfo{};
  auto const extensions = glfw::instance_extensions();
  instance_ci.setPApplicationInfo(&app_info).setPEnabledExtensionNames(extensions);
 
  static constexpr auto layers_v = std::array{
    "VK_LAYER_KHRONOS_shader_object",
  };

  auto const layers = get_layers(layers_v);
  instance_ci.setPEnabledLayerNames(layers);

  m_instance = vk::createInstanceUnique(instance_ci);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);
}

void App::create_surface() {
  m_surface = glfw::create_surface(m_window.get(), *m_instance);
}

void App::select_gpu() {
  m_gpu = get_suitable_gpu(*m_instance, *m_surface);
  std::println("Using GPU: {}", std::string_view{m_gpu.propierties.deviceName});
}

void App::create_device() {
  auto queue_ci = vk::DeviceQueueCreateInfo{};
  static constexpr auto queue_priorities_v = std::array{1.0f};
  queue_ci.setQueueFamilyIndex(m_gpu.queue_family).setQueueCount(1).setQueuePriorities(queue_priorities_v);

  auto enabled_featrues = vk::PhysicalDeviceFeatures{};
  enabled_featrues.fillModeNonSolid = m_gpu.features.fillModeNonSolid;
  enabled_featrues.wideLines = m_gpu.features.wideLines;
  enabled_featrues.samplerAnisotropy = m_gpu.features.samplerAnisotropy;
  enabled_featrues.sampleRateShading = m_gpu.features.sampleRateShading;

  auto sync_feature = vk::PhysicalDeviceSynchronization2Features{vk::True};
  auto dynamic_rendering_feature = vk::PhysicalDeviceDynamicRenderingFeatures{vk::True};
  auto shader_object_feature = 
    vk::PhysicalDeviceShaderObjectFeaturesEXT{vk::True};
  
  dynamic_rendering_feature.setPNext(&shader_object_feature);
  sync_feature.setPNext(&dynamic_rendering_feature);

  auto device_ci = vk::DeviceCreateInfo{};
  static constexpr auto extensions_v = std::array{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_EXT_shader_object",
  };

  device_ci.setPEnabledExtensionNames(extensions_v)
    .setQueueCreateInfos(queue_ci)
    .setPEnabledFeatures(&enabled_featrues)
    .setPNext(&sync_feature);
    
  m_device = m_gpu.device.createDeviceUnique(device_ci);

  VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);
  
  static constexpr std::uint32_t queue_index_v{0};
  m_queue = m_device->getQueue(m_gpu.queue_family, queue_index_v);

  m_waiter = *m_device;



}

void App::create_swapchain() {
  auto const size = glfw::framebuffer_size(m_window.get());
  m_swapchain.emplace(*m_device, m_gpu, *m_surface, size);
}

void App::create_render_sync() {
  auto command_pool_ci = vk::CommandPoolCreateInfo{};
  command_pool_ci.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    .setQueueFamilyIndex(m_gpu.queue_family);
  m_render_cmd_pool = m_device->createCommandPoolUnique(command_pool_ci);

  auto command_buffer_ai = vk::CommandBufferAllocateInfo{};
  command_buffer_ai.setCommandPool(*m_render_cmd_pool)
    .setCommandBufferCount(static_cast<std::uint32_t>(resource_buffering_v))
    .setLevel(vk::CommandBufferLevel::ePrimary);
  auto const command_buffers = 
    m_device->allocateCommandBuffers(command_buffer_ai);
  assert(command_buffers.size() == m_render_sync.size());

  static constexpr auto fence_create_info_v = vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled};
  for (auto [sync, command_buffer] : std::views::zip(m_render_sync, command_buffers)) {
    sync.command_buffer = command_buffer;
    sync.draw = m_device->createSemaphoreUnique({});
    sync.drawn = m_device->createFenceUnique(fence_create_info_v);
  }
}

auto App::acquire_render_target() -> bool {
  m_framebuffer_size = glfw::framebuffer_size(m_window.get());

  if (m_framebuffer_size.x <= 0 || m_framebuffer_size.y <= 0) { return false; }

  auto& render_sync = m_render_sync.at(m_frame_index);
  static constexpr auto fence_timeout_v = 
    static_cast<std::uint64_t>(std::chrono::nanoseconds{std::chrono::seconds{3}}.count());
  auto result =
    m_device->waitForFences(*render_sync.drawn, vk::True, fence_timeout_v);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error{"Failed to wait for render fence"};
  }

  m_render_target = m_swapchain->acquire_next_image(*render_sync.draw);
  if( !m_render_target ) {
    m_swapchain->recreate(m_framebuffer_size);
    return false;
  }

  m_device->resetFences(*render_sync.drawn);
  m_imgui->new_frame();

  return true;
}

auto App::begin_frame() -> vk::CommandBuffer {
  auto const& render_sync = m_render_sync.at(m_frame_index);

  auto command_buffer_bi = vk::CommandBufferBeginInfo{};
  // Only use recorded commands once
  command_buffer_bi.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  render_sync.command_buffer.begin(command_buffer_bi);
  return render_sync.command_buffer;
}

void App::transition_for_render(vk::CommandBuffer const command_buffer) const {
  auto dependency_info = vk::DependencyInfo{};
  auto barrier = m_swapchain->base_barrier();

  barrier.setOldLayout(vk::ImageLayout::eUndefined)
    .setNewLayout(vk::ImageLayout::eAttachmentOptimal)
    .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite)
    .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
    .setDstAccessMask(barrier.srcAccessMask)
    .setDstStageMask(barrier.srcStageMask);
  dependency_info.setImageMemoryBarriers(barrier);
  command_buffer.pipelineBarrier2(dependency_info);
}

void App::inspect() {
  ImGui::ShowDemoWindow();
  // TODO
}

void App::draw(vk::CommandBuffer const command_buffer) const {
  m_shader->bind(command_buffer, m_framebuffer_size);
  command_buffer.draw(3, 1, 0, 0);
}

void App::render(vk::CommandBuffer const command_buffer) {
  auto color_attachment = vk::RenderingAttachmentInfo{};
  color_attachment.setImageView(m_render_target->image_view)
    .setImageLayout(vk::ImageLayout::eAttachmentOptimal)
    .setLoadOp(vk::AttachmentLoadOp::eClear)
    .setStoreOp(vk::AttachmentStoreOp::eStore)
    // Temporaly red
    .setClearValue(vk::ClearColorValue{1.0f, 0.0f, 0.0f, 1.0f});
  auto rendering_info = vk::RenderingInfo{};
  auto const render_area =
    vk::Rect2D{vk::Offset2D{}, m_render_target->extent};
  rendering_info.setRenderArea(render_area)
    .setColorAttachments(color_attachment)
    .setLayerCount(1);


  command_buffer.beginRendering(rendering_info);
  // here is where you draw stuff
  
  inspect();
  draw(command_buffer);

  // ----------------------------
  command_buffer.endRendering();

  m_imgui->end_frame();
  color_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
  rendering_info.setColorAttachments(color_attachment)
    .setPDepthAttachment(nullptr);
  command_buffer.beginRendering(rendering_info);
  m_imgui->render(command_buffer);
  command_buffer.endRendering();
}

void App::transition_for_present(vk::CommandBuffer const command_buffer) const {
  auto dependency_info = vk::DependencyInfo{};
  auto barrier = m_swapchain -> base_barrier();

  barrier.setOldLayout(vk::ImageLayout::eAttachmentOptimal)
    .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
    .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite)
    .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
    .setDstAccessMask(barrier.srcAccessMask)
    .setDstStageMask(barrier.srcStageMask);
  dependency_info.setImageMemoryBarriers(barrier);
  command_buffer.pipelineBarrier2(dependency_info);
}

void App::submit_and_present() {
  auto const& render_sync = m_render_sync.at(m_frame_index);
  render_sync.command_buffer.end();

  auto submit_info = vk::SubmitInfo2{};
  auto const command_buffer_info = 
    vk::CommandBufferSubmitInfo{render_sync.command_buffer};
  auto wait_semaphore_info = vk::SemaphoreSubmitInfo{};
  wait_semaphore_info.setSemaphore(*render_sync.draw)
    .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  auto signal_semaphore_info = vk::SemaphoreSubmitInfo{};
  signal_semaphore_info.setSemaphore(m_swapchain->get_present_semaphore())
    .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  submit_info.setCommandBufferInfos(command_buffer_info)
    .setWaitSemaphoreInfos(wait_semaphore_info)
    .setSignalSemaphoreInfos(signal_semaphore_info);
  m_queue.submit2(submit_info, *render_sync.drawn);

  m_frame_index = (m_frame_index + 1) % m_render_sync.size();
  m_render_target.reset();

  auto const fb_size_changed = m_framebuffer_size != m_swapchain->get_size();
  auto const out_of_date = !m_swapchain->present(m_queue);
  if (fb_size_changed || out_of_date) {
    m_swapchain->recreate(m_framebuffer_size);
  }
}

void App::create_imgui() {
  auto const imgui_ci = DearImGui::CreateInfo{
    .window           = m_window.get(),
    .api_version      = vk_version_v,
    .instance         = *m_instance,
    .physical_device  = m_gpu.device,
    .queue_family     = m_gpu.queue_family,
    .device           = *m_device,
    .queue            = m_queue,
    .color_format     = m_swapchain->get_format(),
    .samples          = vk::SampleCountFlagBits::e1,
  };
  m_imgui.emplace(imgui_ci);
}

void App::create_shader() {
  auto const vert_spirv = to_spir_v(asset_path("shader.vert"));
  auto const frag_spirv = to_spir_v(asset_path("shader.frag"));
  auto const shader_ci = ShaderProgram::CreateInfo{
    .device = *m_device,
    .vert_spirv = vert_spirv,
    .frag_spirv = frag_spirv,
    .set_layouts = {},
    .vertex_input = {},
  };
  m_shader.emplace(shader_ci);
}

auto App::asset_path(std::string_view const uri) const -> fs::path {
  return m_assets_dir / uri;
}

void App::main_loop() {
  while (glfwWindowShouldClose(m_window.get()) == GLFW_FALSE) {
    glfwPollEvents();
    if(!acquire_render_target()) { continue; }
    auto const command_buffer = begin_frame();
    transition_for_render(command_buffer);
    render(command_buffer);
    transition_for_present(command_buffer);
    submit_and_present();
  }
}


}
