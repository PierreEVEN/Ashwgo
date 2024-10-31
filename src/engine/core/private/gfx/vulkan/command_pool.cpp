#include <utility>

#include "gfx/vulkan/command_pool.hpp"

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/queue_family.hpp"

namespace Engine
{
	CommandPool::CommandPool(std::weak_ptr<Device> in_device, const uint32_t& queue_family) : device(std::move(in_device))
	{
		VkCommandPoolCreateInfo create_infos{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queue_family
		};
		VK_CHECK(vkCreateCommandPool(device.lock()->raw(), &create_infos, nullptr, &ptr), "Failed to create command pool")
	}

	CommandPool::~CommandPool()
	{
		vkDestroyCommandPool(device.lock()->raw(), ptr, nullptr);
	}

	VkCommandBuffer CommandPool::allocate() const
	{
		VkCommandBufferAllocateInfo infos{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = ptr,
			.level= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		VkCommandBuffer out;
		VK_CHECK(vkAllocateCommandBuffers(device.lock()->raw(), &infos, &out), "failed to allocate command buffer");
		return out;
	}

	void CommandPool::free(VkCommandBuffer command_buffer) const
	{
		vkFreeCommandBuffers(device.lock()->raw(), ptr, 1, &command_buffer);
	}
}
