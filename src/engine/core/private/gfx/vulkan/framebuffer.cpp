#include "gfx/vulkan/framebuffer.hpp"

#include "gfx/renderer/renderer.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/image_view.hpp"

namespace Engine
{
	Framebuffer::Framebuffer(std::weak_ptr<Device> in_device, const RenderPassInstanceBase& render_pass,
	                         size_t image_index) : device(std::move(in_device)),
	                                               command_buffer(
		                                               std::make_shared<CommandBuffer>(
			                                               device, QueueSpecialization::Graphic))
	{
		const auto rp = render_pass.get_render_pass().lock();

		std::vector<VkImageView> views;
		for (const auto& view : render_pass.get_attachments())
			views.emplace_back(view.lock()->raw()[image_index]);


		assert(!render_pass.get_attachments().empty());

		LOG_WARNING("Framebuffer : {} => {}", (size_t)rp->raw(), static_cast<uint32_t>(views.size()));
		VkFramebufferCreateInfo create_infos = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = rp->raw(),
			.attachmentCount = static_cast<uint32_t>(views.size()),
			.pAttachments = views.data(),
			.width = static_cast<uint32_t>(render_pass.resolution().x),
			.height = static_cast<uint32_t>(render_pass.resolution().y),
			.layers = 1,
		};

		VK_CHECK(vkCreateFramebuffer(device.lock()->raw(), &create_infos, nullptr, &ptr),
		         "Failed to create render pass");
	}

	Framebuffer::~Framebuffer()
	{
		vkDestroyFramebuffer(device.lock()->raw(), ptr, nullptr);
	}
}
