#pragma once
#include <memory>
#include <vector>

#include "device.hpp"

namespace Engine
{
	class BufferResource;
	class Device;

	enum class EBufferType
	{
		IMMUTABLE, // No updates allowed
		STATIC, // Pretty never updated. Updating data would cause some freezes (low memory footprint)
		DYNAMIC,
		// Data is stored internally, then automatically submitted. Can lead to a memory overhead depending on the get size.
		IMMEDIATE, // Data need to be submitted every frames
	};

	enum class EBufferUsage
	{
		INDEX_DATA = 0x00000001, // used as index get
		VERTEX_DATA = 0x00000002, // used as vertex get
		GPU_MEMORY = 0x00000003, // used as storage get
		UNIFORM_BUFFER = 0x00000004, // used as uniform get
		INDIRECT_DRAW_ARGUMENT = 0x00000005, // used for indirect draw commands
		TRANSFER_MEMORY = 0x00000006, // used for indirect draw commands
	};

	enum class EBufferAccess
	{
		DEFAULT = 0x00000000, // Choose best configuration
		GPU_ONLY = 0x00000001, // Data will be cached on GPU
		CPU_TO_GPU = 0x00000002, // frequent transfer from CPU to GPU
		GPU_TO_CPU = 0x00000003, // frequent transfer from GPU to CPU
	};

	class BufferData
	{
	public:
		BufferData() : data(nullptr), element_count(0), stride(0)
		{
		}

		template <typename T>
		BufferData(const T& object) : BufferData(&object, sizeof(T), 1)
		{
		}

		BufferData(const void* in_data, size_t in_stride, size_t in_element_count) : data(const_cast<void*>(in_data)),
			element_count(in_element_count),
			stride(in_stride)
		{
		}

		BufferData(BufferData&) = delete;

		~BufferData()
		{
			if (own_data)
				free(data);
		}

		BufferData copy() const
		{
			void* new_data = nullptr;
			memcpy(new_data, data, stride * element_count);
			BufferData buffer = BufferData(new_data, stride, element_count);
			buffer.own_data = true;
			return buffer;
		}

		size_t get_stride() const { return stride; }
		size_t get_element_count() const { return element_count; }

		void copy_to(void* destination) const;

	private:
		bool own_data = false;
		void* data = nullptr;
		size_t element_count = 0;
		size_t stride = 0;
	};

	class Buffer
	{
	public:
		struct CreateInfos
		{
			EBufferUsage usage;
			EBufferAccess access = EBufferAccess::DEFAULT;
			EBufferType type = EBufferType::IMMUTABLE;
			size_t stride = 0;
			size_t element_count = 0;
		};

		Buffer(std::weak_ptr<Device> device, const CreateInfos& create_infos);
		Buffer(std::weak_ptr<Device> device, const CreateInfos& create_infos, const BufferData& data);
		~Buffer();

		bool resize(size_t stride, size_t element_count);

		void set_data(const BufferData& data);

		std::vector<VkBuffer> raw() const;
		VkBuffer raw_current();

	private:
		CreateInfos params;
		size_t element_count = 0;
		size_t stride = 0;
		BufferData temp_buffer_data;
		std::vector<std::shared_ptr<BufferResource>> buffers;
		std::weak_ptr<Device> device;
	};


	class BufferResource : public DeviceResource
	{
	public:
		BufferResource(std::weak_ptr<Device> device, const Buffer::CreateInfos& create_infos);
		~BufferResource();
		void set_data(const BufferData& data);

		bool outdated = false;
		VkBuffer ptr = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
	};
}
