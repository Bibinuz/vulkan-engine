#pragma once

#include "glm/fwd.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <array>
#include <cstddef>
namespace lvk {

struct Vertex {
    glm::vec2 position{};
    glm::vec3 color{1.0f};
    glm::vec2 uv{};
};

constexpr auto vertex_attributes_v = std::array{
    vk::VertexInputAttributeDescription2EXT{0, 0, vk::Format::eR32G32Sfloat,
                                            offsetof(Vertex, position)},
    vk::VertexInputAttributeDescription2EXT{1, 0, vk::Format::eR32G32B32Sfloat,
                                            offsetof(Vertex, color)},
    vk::VertexInputAttributeDescription2EXT{2, 0, vk::Format::eR32G32Sfloat, //
                                            offsetof(Vertex, uv)},
};

constexpr auto vertex_bindings_v = std::array{
    vk::VertexInputBindingDescription2EXT{0, sizeof(Vertex), vk::VertexInputRate::eVertex, 1},
};

} // namespace lvk
