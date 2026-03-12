#pragma once
#include "vulkan/vulkan_handles.hpp"
#include "window.hpp"
#include "gpu.hpp"

namespace lvk {
class App {
  public:
    void run();
  
  private:  
    glfw::Window m_window{};
    vk::UniqueInstance m_instance{};
    vk::UniqueSurfaceKHR m_surface{};
    Gpu m_gpu{};
    

    void create_window();
    void create_instance();
    void create_surface();
    void select_gpu();

    void main_loop();

};
}
