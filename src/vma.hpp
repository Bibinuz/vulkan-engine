#pragma once

#include <cstddef>
#include <vk_mem_alloc.h>
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "scoped.hpp"

namespace lvk::vma {

struct Deleter{
  void operator()(VmaAllocator allocator) const noexcept;
};

struct RawBuffer {
  [[nodiscard]] auto mapped_span() const -> std::span<std::byte> {
    return std::span{static_cast<std::byte*>(mapped), size};
  }
  auto operator ==(RawBuffer const& rhs) const -> bool = default;

  VmaAllocator allocator{};
  VmaAllocation allocation{};
  vk::Buffer buffer{};
  vk::DeviceSize size{};
  void* mapped{};
};

struct BufferDeleter {
  void operator()(RawBuffer const& raw_buffer) const noexcept;
};

struct BufferCreateInfo {
  VmaAllocator allocator;
  vk::BufferUsageFlags usage;
  std::uint32_t queue_family;
};

enum class BufferMemoryType : std::int8_t {Host, Device};

using Allocator = Scoped<VmaAllocator, Deleter>;
using Buffer = Scoped<RawBuffer, BufferDeleter>;

[[nodiscard]] auto create_allocator(vk::Instance instance,
                                    vk::PhysicalDevice physical_device,
                                    vk::Device device) -> Scoped<VmaAllocator, Deleter>;


[[nodiscard]] auto create_buffer(BufferCreateInfo const& create_info,
                                 BufferMemoryType memory_type,
                                 vk::DeviceSize size) -> Scoped<RawBuffer, BufferDeleter>;

}
