#pragma once

#include <cstddef>
#include <span>
#include <vk_mem_alloc.h>
#include "commandblock.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "scoped.hpp"
#include "vulkan/vulkan_structs.hpp"

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

struct RawImage {
  auto operator==(RawImage const& rhs) const -> bool = default;

  VmaAllocator allocator{};
  VmaAllocation allocation{};
  vk::Image image{};
  vk::Extent2D extent{};
  vk::Format format{};
  std::uint32_t levels{};
};

struct ImageDeleter {
  void operator()(RawImage const& raw_image) const noexcept;
};

struct ImageCreateInfo {
  VmaAllocator allocator;
  std::uint32_t queue_family;
};

struct Bitmap {
  std::span<std::byte const> bytes{};
  glm::ivec2 size{};
};

using Allocator = Scoped<VmaAllocator, Deleter>;
using Buffer = Scoped<RawBuffer, BufferDeleter>;
using ByteSpans = std::span<std::span<std::byte const> const>;
using Image = Scoped<RawImage, ImageDeleter>;

[[nodiscard]] auto create_allocator(vk::Instance instance,
                                    vk::PhysicalDevice physical_device,
                                    vk::Device device) -> Scoped<VmaAllocator, Deleter>;


[[nodiscard]] auto create_buffer(BufferCreateInfo const& create_info,
                                 BufferMemoryType memory_type,
                                 vk::DeviceSize size) -> Scoped<RawBuffer, BufferDeleter>;

[[nodiscard]] auto create_device_buffer(BufferCreateInfo const& create_info,
                                        CommandBlock command_block,
                                        ByteSpans const& byte_spans) -> Buffer;

[[nodiscard]] auto create_image(ImageCreateInfo const& create_info,
                                vk::ImageUsageFlags usage, std::uint32_t levels,
                                vk::Format format, vk::Extent2D extent) -> Image;

[[nodiscard]] auto create_sampled_image(ImageCreateInfo const& create_info,
                                        CommandBlock command_block,
                                        Bitmap const& bitmap) -> Image;
} // End namespace lvk
