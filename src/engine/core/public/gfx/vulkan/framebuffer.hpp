#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Engine
{
	class Semaphore;
	class RenderPassInstanceBase;
	class Device;
}

namespace Engine
{
	class CommandBuffer;

	class Framebuffer
	{
	public:

		Framebuffer(std::weak_ptr<Device> device, const RenderPassInstanceBase& render_pass, size_t image_index);
		Framebuffer(Framebuffer&&) = delete;
		Framebuffer(Framebuffer&) = delete;
		~Framebuffer();

		CommandBuffer& get_command_buffer() const { return *command_buffer; }

		VkFramebuffer raw() const { return ptr; }

		Semaphore& render_finished_semaphore() const { return *render_finished_semaphores; }

	private:

		std::shared_ptr<Semaphore> render_finished_semaphores;
		std::weak_ptr<Device> device;
		VkFramebuffer ptr = VK_NULL_HANDLE;
		std::shared_ptr<CommandBuffer> command_buffer;
	};
}
