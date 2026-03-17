#include "app.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_structs.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <print>
#include <string_view>
#include <vulkan/vulkan_hpp_macros.hpp>
#include "gpu.hpp"
#include <ranges>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE


namespace lvk {

void App::run() {
  create_window();
  create_instance();
  create_surface();
  select_gpu();  
  create_device();
  create_swapchain();
  create_render_sync();
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

  sync_feature.setPNext(&dynamic_rendering_feature);

  auto device_ci = vk::DeviceCreateInfo{};
  static constexpr auto extensions_v = std::array{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
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

void App::main_loop() {
  while (glfwWindowShouldClose(m_window.get()) == GLFW_FALSE) {
    glfwPollEvents();
  }
}


}
