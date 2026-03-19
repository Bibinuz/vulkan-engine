#include "vma.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_hpp_macros.hpp"
#include <print>
#include <stdexcept>

namespace lvk{
namespace vma{

void Deleter::operator()(VmaAllocator allocator) const noexcept{
  vmaDestroyAllocator(allocator);
}

void BufferDeleter::operator()(RawBuffer const& raw_buffer) const noexcept {
  vmaDestroyBuffer(raw_buffer.allocator, raw_buffer.buffer, raw_buffer.allocation);
}

} // end namespace vma

auto vma::create_allocator(vk::Instance const instance, 
                           vk::PhysicalDevice const physical_device, 
                           vk::Device const device) -> Allocator {
  auto const& dispatcher = VULKAN_HPP_DEFAULT_DISPATCHER;
  auto vma_vk_funcs = VmaVulkanFunctions{};
  vma_vk_funcs.vkGetInstanceProcAddr = dispatcher.vkGetInstanceProcAddr;
  vma_vk_funcs.vkGetDeviceProcAddr = dispatcher.vkGetDeviceProcAddr;

  auto allocator_ci = VmaAllocatorCreateInfo{};
  allocator_ci.physicalDevice = physical_device;
  allocator_ci.device = device;
  allocator_ci.pVulkanFunctions = &vma_vk_funcs;
  allocator_ci.instance = instance;
  VmaAllocator ret{};
  auto const result = vmaCreateAllocator(&allocator_ci, &ret);
  if (result == VK_SUCCESS) { return ret; }

  throw std::runtime_error{"Failed to create Vulkan memory Allocator"};
}

auto vma::create_buffer(BufferCreateInfo const& create_info, 
                        BufferMemoryType const memory_type, 
                        vk::DeviceSize const size) -> Buffer{
  if (size == 0) {
    std::println(stderr, "Buffer cannot be 0-sized");
    return {};
  }

  auto allocation_ci = VmaAllocationCreateInfo{};
  allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  vk::BufferUsageFlags usage = create_info.usage;
  if (memory_type == BufferMemoryType::Device) {
    allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    usage |= vk::BufferUsageFlagBits::eTransferDst;
  } else {
    allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocation_ci.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
  }

  auto buffer_ci = vk::BufferCreateInfo{};
  buffer_ci.setQueueFamilyIndices(create_info.queue_family)
    .setSize(size)
    .setUsage(usage);
  auto vma_buffer_ci = static_cast<VkBufferCreateInfo>(buffer_ci);

  VmaAllocation allocation{};
  VkBuffer buffer{};
  auto allocation_info = VmaAllocationInfo{};
  auto const result =
    vmaCreateBuffer(create_info.allocator, &vma_buffer_ci, &allocation_ci,
                    &buffer, &allocation, &allocation_info);
  if (result != VK_SUCCESS) {
    std::println(stderr, "Failed to create VMA Buffer");
    return {};
  }
  return RawBuffer{
    .allocator = create_info.allocator,
    .allocation = allocation,
    .buffer = buffer,
    .size = size,
    .mapped = allocation_info.pMappedData,
  };
}

} //end namespace lvk

