#pragma once
#include <memory>
#include <string>
#include <glm/vec2.hpp>

namespace Engine
{
	class PresentStep;
}

namespace Engine
{
	class Device;
	class Instance;
	class Surface;
}

struct GLFWwindow;

namespace Engine
{
	class RendererStep;

	struct WindowConfig
	{
		std::string name = "no name";
		glm::uvec2 resolution = {800, 600};
	};


	class Window : public std::enable_shared_from_this<Window>
	{
	public:
		~Window();

		GLFWwindow* raw() const { return ptr; }

		size_t get_id() const { return id; }

		bool render();

		glm::uvec2 internal_extent() const;

		void close()
		{
			should_close = true;
		}

		void set_renderer(const std::shared_ptr<PresentStep>& present_pass) const;

		std::shared_ptr<Surface> get_surface() const { return surface; }

		static std::shared_ptr<Window> create(const std::weak_ptr<Instance>& instance, const WindowConfig& config);

	private:

		Window(const WindowConfig& config);
		std::shared_ptr<Surface> surface;
		size_t id;
		bool should_close = false;
		GLFWwindow* ptr;
	};
}
