#pragma once
#include "gfx/renderer/definition/render_pass_id.hpp"
#include "gfx/renderer/definition/renderer.hpp"
#include "logger.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/device_resource.hpp"

#include <ankerl/unordered_dense.h>
#include <glm/vec2.hpp>
#include <memory>

namespace Eng::Gfx
{
class Buffer;
class IRenderPass;
class CommandBuffer;
class CustomPassList;
class Framebuffer;
class ImGuiWrapper;
class VkRendererPass;
class RenderPassInstance;
class ImageView;
class Semaphore;
class Fence;
class Renderer;
class Device;

using SwapchainImageId = uint8_t;
using DeviceImageId    = uint8_t;

struct FrameResources
{
    ankerl::unordered_dense::map<std::string, std::shared_ptr<ImageView>> images;
    ankerl::unordered_dense::map<std::string, std::shared_ptr<Buffer>>    buffers;

    std::vector<std::shared_ptr<Framebuffer>> framebuffer;
};

class RenderPassInstanceBase : public DeviceResource
{
public:
    static std::shared_ptr<RenderPassInstanceBase> create(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref);

    static std::shared_ptr<RenderPassInstanceBase> create(std::weak_ptr<Device> device, const Renderer& renderer)
    {
        return create(std::move(device), renderer, *renderer.root_node());
    }

    // Should be called before each frame to reset max draw flags
    void                    reset_for_next_frame();
    virtual FrameResources* create_or_resize(const glm::uvec2& viewport, const glm::uvec2& parent, bool b_force = false);
    void                    render(SwapchainImageId swapchain_image, DeviceImageId device_image);

    /**
     * The resolution of this current pass
     */
    const glm::uvec2& resolution() const
    {
        return current_resolution;
    }

    /**
     * The internal resolution of the platform window
     */
    const glm::uvec2& viewport_resolution() const
    {
        return viewport_res;
    }

    // Get draw pass definition
    const RenderNode& get_definition() const
    {
        return definition;
    }

    /**
     * Iterate over dependencies
     */
    void for_each_dependency(const std::function<void(const std::shared_ptr<RenderPassInstanceBase>&)>& callback) const;

    /**
     * Iterate over dependencies that match the given filter
     */
    void for_each_dependency(const RenderPassGenericId& id, const std::function<void(const std::shared_ptr<RenderPassInstanceBase>&)>& callback) const;

    // Find child dependency by reference
    std::weak_ptr<RenderPassInstanceBase> get_dependency(const RenderPassRef& ref) const;

    // Find the image for the given input attachment name
    std::weak_ptr<ImageView> get_image_resource(const std::string& resource_name) const;
    std::weak_ptr<Buffer>    get_buffer_resource(const std::string& resource_name) const;

    void set_resize_callback(const RenderNode::ResizeCallback& in_callback)
    {
        definition.resize_callback(in_callback);
    }

    std::weak_ptr<CustomPassList> get_custom_passes()
    {
        return custom_passes;
    }

    uint32_t get_current_image_index() const
    {
        return current_framebuffer_index;
    }

protected:
    RenderPassInstanceBase(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref);

    // Retrieve the fence that will be signaled once the image rendering is finished
    const Fence* get_render_finished_fence(DeviceImageId device_image) const;

    bool enable_parallel_rendering() const
    {
        return render_pass_interface && render_pass_interface->record_threads() > 1;
    }

    virtual std::shared_ptr<ImageView> create_view_for_attachment(const std::string& attachment);
    virtual void                       render_internal(SwapchainImageId swapchain_image, DeviceImageId device_image) = 0;

    std::shared_ptr<IRenderPass> render_pass_interface;

    virtual uint8_t get_framebuffer_count() const;
    virtual void    fill_command_buffer(CommandBuffer& cmd, size_t group_index) const;

private:
    bool prepared  = false;
    bool submitted = false;

    std::optional<FrameResources> frame_resources;
    std::optional<FrameResources> next_frame_resources;

    std::vector<std::shared_ptr<Semaphore>> render_finished_semaphores;
    std::vector<std::shared_ptr<Fence>>     render_finished_fences;

    ankerl::unordered_dense::map<RenderPassRef, std::shared_ptr<RenderPassInstanceBase>> dependencies;
    std::shared_ptr<CustomPassList>                                                      custom_passes;

    glm::uvec2 viewport_res{0, 0};
    glm::uvec2 current_resolution{0, 0};
    RenderNode definition;
    uint32_t   current_framebuffer_index = 0;
};

class CustomPassList
{
public:
    CustomPassList(const std::weak_ptr<Device>& in_device) : device(in_device)
    {
    }

    std::shared_ptr<RenderPassInstanceBase> add_custom_pass(const std::vector<RenderPassGenericId>& targets, const Renderer& renderer);

    void remove_custom_pass(const RenderPassRef& ref);

    /**
     * Iterate over dependencies
     */
    void for_each_dependency(const RenderPassGenericId& target_id, const std::function<void(const std::shared_ptr<RenderPassInstanceBase>&)>& callback) const;

    // Find child dependency by reference
    std::weak_ptr<RenderPassInstanceBase> get_dependency(const RenderPassGenericId& target_id, const RenderPassRef& ref) const;

private:
    std::weak_ptr<Device>                                                                                                                   device;
    ankerl::unordered_dense::map<RenderPassGenericId, ankerl::unordered_dense::map<RenderPassRef, std::shared_ptr<RenderPassInstanceBase>>> temporary_dependencies;
};
} // namespace Eng::Gfx