#include "engine.hpp"

#include <iostream>
#include <GLFW/glfw3.h>

#include "config.hpp"
#include "gfx/window.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/physical_device.hpp"
#include "gfx/vulkan/queue_family.hpp"
#include "gfx/vulkan/surface.hpp"


namespace Engine {
    Engine::Engine(Config& config)
    {
        glfwInit();

        

        gfx_instance = std::make_shared<Instance>(config);

        auto physical_device = PhysicalDevice::pick_best_physical_device(*gfx_instance, config);
        if (!physical_device)
            LOG_FATAL("%s", physical_device.error().c_str());
        std::cout << "selected physical device " << physical_device.get().get_device_name() << std::endl;

        gfx_device= std::make_shared<Device>(config, physical_device.get());



    }

    Engine::~Engine()
    {
        glfwTerminate();
    }

    std::shared_ptr<Window> Engine::new_window(const WindowConfig& config)
    {
        bool first_surface = false;
        if (windows.empty())
            first_surface = true;

        const auto window = std::make_shared<Window>(config);

        const auto surface = std::make_shared<Surface>(gfx_instance, *window);


        if (first_surface)
            gfx_device->queues->init_first_surface(*surface, gfx_device->get_physical_device());
        windows.emplace(reinterpret_cast<size_t>(window->raw()), window);
        return window;
    }
}