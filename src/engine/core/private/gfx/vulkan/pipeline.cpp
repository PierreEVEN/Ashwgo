#include "gfx/vulkan/pipeline.hpp"

#include "gfx/renderer/renderer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "gfx/vulkan/vk_check.hpp"

namespace Engine
{
	static VkDescriptorType vk_descriptor_type(EBindingType type)
	{
		switch (type)
		{
		case EBindingType::SAMPLER:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case EBindingType::COMBINED_IMAGE_SAMPLER:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case EBindingType::SAMPLED_IMAGE:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case EBindingType::STORAGE_IMAGE:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case EBindingType::UNIFORM_TEXEL_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case EBindingType::STORAGE_TEXEL_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case EBindingType::UNIFORM_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case EBindingType::STORAGE_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case EBindingType::UNIFORM_BUFFER_DYNAMIC:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case EBindingType::STORAGE_BUFFER_DYNAMIC:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		case EBindingType::INPUT_ATTACHMENT:
			return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		default:;
			LOG_FATAL("unhandled case");
		}
	}

	static VkPolygonMode vk_polygon_mode(EPolygonMode polygon_mode)
	{
		switch (polygon_mode)
		{
		case EPolygonMode::Point:
			return VK_POLYGON_MODE_POINT;
		case EPolygonMode::Line:
			return VK_POLYGON_MODE_LINE;
		case EPolygonMode::Fill:
			return VK_POLYGON_MODE_FILL;
		default:
			LOG_FATAL("unhandled case");
		}
	}

	static VkPrimitiveTopology vk_topology(ETopology topology)
	{
		switch (topology)
		{
		case ETopology::Points:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case ETopology::Lines:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case ETopology::Triangles:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		default:
			LOG_FATAL("unhandled case");
		}
	}

	static VkFrontFace vk_front_face(EFrontFace front_face)
	{
		switch (front_face)
		{
		case EFrontFace::Clockwise:
			return VK_FRONT_FACE_CLOCKWISE;
		case EFrontFace::CounterClockwise:
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		default:
			LOG_FATAL("unhandled case");
		}
	}

	static VkCullModeFlags vk_cull_mode(ECulling culling)
	{
		switch (culling)
		{
		case ECulling::None:
			return VK_CULL_MODE_NONE;
		case ECulling::Front:
			return VK_CULL_MODE_FRONT_BIT;
		case ECulling::Back:
			return VK_CULL_MODE_BACK_BIT;
		case ECulling::Both:
			return VK_CULL_MODE_FRONT_AND_BACK;
		default:
			LOG_FATAL("unhandled case");
		}
	}

	Pipeline::Pipeline(std::weak_ptr<Device> device, std::shared_ptr<RenderPassObject> render_pass,
	                   std::vector<VertexInput> vertex_inputs,
	                   std::vector<std::shared_ptr<ShaderModule>> shader_stage,
	                   const CreateInfos& create_infos)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (const auto& stage : shader_stage)
		{
			for (const auto& binding : stage->get_bindings()) {
				bindings.emplace_back(VkDescriptorSetLayoutBinding{
					.binding = binding.binding,
					.descriptorType = vk_descriptor_type(binding.type),
					.descriptorCount = 1,
					.stageFlags = static_cast<VkShaderStageFlags>(stage->infos().stage),
					.pImmutableSamplers = nullptr,
					});
			}
		}

		VkDescriptorSetLayoutCreateInfo layout_infos{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = NULL,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data(),
		};

		VK_CHECK(vkCreateDescriptorSetLayout(device.lock()->raw(), &layout_infos, nullptr, &descriptor_set_layout),
		         "Failed to create descriptor set layout");


		std::vector<VkPushConstantRange> push_constants = {};
		for (const auto& stage : shader_stage)
			if (stage->infos().push_constant_size > 0)
				push_constants.emplace_back(VkPushConstantRange{
					.stageFlags = static_cast<VkShaderStageFlags>(stage->infos().stage),
					.offset = 0,
					.size = stage->infos().push_constant_size,
				});

		const VkPipelineLayoutCreateInfo pipeline_layout_infos{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &descriptor_set_layout,
			.pushConstantRangeCount = static_cast<uint32_t>(push_constants.size()),
			.pPushConstantRanges = push_constants.data(),
		};
		VK_CHECK(vkCreatePipelineLayout(device.lock()->raw(), &pipeline_layout_infos, nullptr, &layout),
		         "Failed to create pipeline layout");


		std::vector<VkVertexInputAttributeDescription> vertex_attribute_description;

		uint32_t vertex_input_size = 0;
		for (const auto& input_property : vertex_inputs)
		{
			if (input_property.location == -1)
				continue;

			vertex_attribute_description.emplace_back(VkVertexInputAttributeDescription{
				.location = static_cast<uint32_t>(input_property.location),
				.format = static_cast<VkFormat>(input_property.format),
				.offset = input_property.offset,
			});

			vertex_input_size += get_format_channel_count(input_property.format) * get_format_bytes_per_pixel(input_property.format);
		}

		const VkVertexInputBindingDescription bindingDescription{
			.binding = 0,
			.stride = vertex_input_size,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		const VkPipelineVertexInputStateCreateInfo vertex_input_state{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.stride > 0 ? 1 : 0),
			.pVertexBindingDescriptions = bindingDescription.stride > 0 ? &bindingDescription : nullptr,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_description.size()),
			.pVertexAttributeDescriptions = vertex_attribute_description.data(),
		};

		const VkPipelineInputAssemblyStateCreateInfo input_assembly{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = vk_topology(create_infos.topology),
			.primitiveRestartEnable = VK_FALSE,
		};

		const VkPipelineViewportStateCreateInfo viewport_state{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1,
		};

		const VkPipelineRasterizationStateCreateInfo rasterizer{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = vk_polygon_mode(create_infos.polygon_mode),
			.cullMode = vk_cull_mode(create_infos.culling),
			.frontFace = vk_front_face(create_infos.front_face),
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = create_infos.line_width,
		};

		const VkPipelineMultisampleStateCreateInfo multisampling{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
		};

		const VkPipelineDepthStencilStateCreateInfo depth_stencil{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = create_infos.depth_test,
			.depthWriteEnable = create_infos.depth_test,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};

		std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachment;
		for (const auto& attachment : render_pass->get_infos().attachments)
		{
			if (attachment.is_depth())
			{
			}
			else
			{
				color_blend_attachment.emplace_back(VkPipelineColorBlendAttachmentState{
					.blendEnable = create_infos.alpha_mode == EAlphaMode::Opaque ? VK_FALSE : VK_TRUE,
					.srcColorBlendFactor = create_infos.alpha_mode == EAlphaMode::Opaque
						                       ? VK_BLEND_FACTOR_ONE
						                       : VK_BLEND_FACTOR_SRC_ALPHA,
					.dstColorBlendFactor = create_infos.alpha_mode == EAlphaMode::Opaque
						                       ? VK_BLEND_FACTOR_ZERO
						                       : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
					.colorBlendOp = VK_BLEND_OP_ADD,
					.srcAlphaBlendFactor = create_infos.alpha_mode == EAlphaMode::Opaque
						                       ? VK_BLEND_FACTOR_ONE
						                       : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
					.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
					.alphaBlendOp = VK_BLEND_OP_ADD,
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
					VK_COLOR_COMPONENT_A_BIT,
				});
			}
		}

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		for (const auto& stage : shader_stage)
		{
			shaderStages.emplace_back(VkPipelineShaderStageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = static_cast<VkShaderStageFlagBits>(stage->infos().stage),
				.module = stage->raw(),
				.pName = stage->infos().entry_point.c_str(),
			});
		}

		VkPipelineColorBlendStateCreateInfo color_blending{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = static_cast<uint32_t>(color_blend_attachment.size()),
			.pAttachments = color_blend_attachment.data(),
		};

		std::vector dynamic_states_array = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};
		if (create_infos.line_width != 1.0f)
			dynamic_states_array.emplace_back(VK_DYNAMIC_STATE_LINE_WIDTH);

		VkPipelineDynamicStateCreateInfo dynamic_states{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamic_states_array.size()),
			.pDynamicStates = dynamic_states_array.data(),
		};

		VkGraphicsPipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = static_cast<uint32_t>(shaderStages.size()),
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertex_input_state,
			.pInputAssemblyState = &input_assembly,
			.pViewportState = &viewport_state,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depth_stencil,
			.pColorBlendState = &color_blending,
			.pDynamicState = &dynamic_states,
			.layout = layout,
			.renderPass = render_pass->raw(),
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1,
		};

		VK_CHECK(vkCreateGraphicsPipelines(device.lock()->raw(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ptr),
		         "Failed to create material graphic pipeline");
	}

	Pipeline::~Pipeline()
	{
		vkDestroyDescriptorSetLayout(device.lock()->raw(), descriptor_set_layout, nullptr);
		vkDestroyPipeline(device.lock()->raw(), ptr, nullptr);
		vkDestroyPipelineLayout(device.lock()->raw(), layout, nullptr);
	}
}
