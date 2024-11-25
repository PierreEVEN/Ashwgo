#pragma once
#include "asset_base.hpp"
#include "object_ptr.hpp"
#include "assets\material_instance_asset.gen.hpp"
#include "gfx/vulkan/pipeline.hpp"

namespace Eng
{
struct MaterialPermutation;
}

namespace Eng::Gfx
{
class Buffer;
class BufferData;
}

namespace Eng
{
class SamplerAsset;
class TextureAsset;
}

namespace Eng
{
class MaterialAsset;

namespace Gfx
{
class Pipeline;
class DescriptorSet;
}

class MaterialInstanceAsset : public AssetBase
{
    REFLECT_BODY()

public:
    MaterialInstanceAsset(const TObjectRef<MaterialAsset>& base_material, bool b_static = true);

    const std::shared_ptr<Gfx::Pipeline>&      get_base_resource(const std::string& shader_pass);
    const std::shared_ptr<Gfx::DescriptorSet>& get_descriptor_resource(const std::string& shader_pass);

    void set_sampler(const std::string& binding, const TObjectRef<SamplerAsset>& sampler);
    void set_texture(const std::string& binding, const TObjectRef<TextureAsset>& texture);
    void set_buffer(const std::string& binding, const std::weak_ptr<Gfx::Buffer>& buffer);
    void set_scene_data(const std::weak_ptr<Gfx::Buffer>& buffer);

private:
    std::weak_ptr<Gfx::Buffer>                                           scene_buffer_data;
    TObjectRef<MaterialAsset>                                            base;
    bool                                                                 b_static = true;
    std::unordered_map<std::string, std::shared_ptr<Gfx::DescriptorSet>> descriptors;

    Gfx::PermutationDescription permutation_description;
    std::weak_ptr<MaterialPermutation> permutation;

    std::unordered_map<std::string, TObjectRef<SamplerAsset>>   samplers;
    std::unordered_map<std::string, TObjectRef<TextureAsset>>   textures;
    std::unordered_map<std::string, std::weak_ptr<Gfx::Buffer>> buffers;
};
} // namespace Eng