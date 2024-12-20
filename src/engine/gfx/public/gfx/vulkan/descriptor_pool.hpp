#pragma once
#include <memory>
#include <mutex>

#include "descriptor_sets.hpp"

namespace Eng::Gfx
{
class PipelineLayout;

struct PoolDescription
{
  public:
    PoolDescription(const PipelineLayout& pipeline);

    bool operator==(const PoolDescription& other) const
    {
        auto a = other.pool_sizes.begin(), b = pool_sizes.begin();
        for (; a != other.pool_sizes.end() && b != pool_sizes.end(); ++a, ++b)
            if (a->descriptorCount != b->descriptorCount || a->type != b->type)
                return false;
        return a == other.pool_sizes.end() && b == pool_sizes.end();
    }

    std::vector<VkDescriptorPoolSize> pool_sizes;
};
} // namespace Eng::Gfx

template <> struct std::hash<Eng::Gfx::PoolDescription>
{
    size_t operator()(const Eng::Gfx::PoolDescription& val) const noexcept
    {
        auto ite = val.pool_sizes.begin();
        if (ite == val.pool_sizes.end())
            return 0;
        size_t hash = std::hash<int32_t>()(static_cast<uint32_t>(ite->type) + ite->descriptorCount + 1);
        for (; ite != val.pool_sizes.end(); ++ite)
        {
            hash ^= std::hash<int32_t>()(static_cast<uint32_t>(ite->type) + ite->descriptorCount) * 13;
        }
        return hash;
    }
};

namespace Eng::Gfx
{
class Device;

class DescriptorPool
{
  public:
    static std::shared_ptr<DescriptorPool> create(std::weak_ptr<Device> device)
    {
        return std::shared_ptr<DescriptorPool>(new DescriptorPool(std::move(device)));
    }

    DescriptorPool(DescriptorPool&)  = delete;
    DescriptorPool(DescriptorPool&&) = delete;
    VkDescriptorSet allocate(const PipelineLayout& pipeline, size_t& pool_index);
    void            free(const VkDescriptorSet& desc_set, const PipelineLayout& pipeline, size_t pool_index);

  private:
    DescriptorPool(std::weak_ptr<Device> device);

    class Pool
    {
      public:
        Pool(std::weak_ptr<Device> device, const PoolDescription& description);
        ~Pool();

        // Return null if full
        VkDescriptorSet allocate(const VkDescriptorSetLayout& layout);

        // Return true if empty
        bool free(const VkDescriptorSet& desc_set);

        // Return true if empty
        bool is_empty() const
        {
            return space_left == initial_space;
        }

      private:
        uint32_t              space_left    = 0;
        uint32_t              initial_space = 0;
        VkDescriptorPool      ptr           = VK_NULL_HANDLE;
        std::weak_ptr<Device> device;
    };

    std::mutex pool_lock;

    std::weak_ptr<Device>                                                                      device;
    ankerl::unordered_dense::map<PoolDescription, std::pair<size_t, std::vector<std::shared_ptr<Pool>>>> pools;
};
} // namespace Eng::Gfx
