#pragma once

#include "mesh_material/Material.h"
#include "mesh_material/Texture.h"

#include <MaterialXCore/Document.h>

namespace mesh_material
{
    enum class PbrTextureSlot : std::uint8_t { BaseColor, MetallicRoughness, Normal, Occlusion, Emissive };
    struct PbrTextureReference
    {
        AssetId asset{};
        std::string sourceUri;
        bool srgb{};
        TextureRequestMetadata metadata{};
        [[nodiscard]] TextureRequest ToTextureRequest(PbrTextureSlot slot) const;
    };
    struct PbrMaterialRuntime
    {
        MaterialX::DocumentPtr document;
        PbrMaterialData pbr;
        Float3 emissive{};
        float normalScale{1.f};
        float occlusionStrength{1.f};
        std::array<PbrTextureReference, 5> textures{};
    };
    class MaterialXRuntimeCompiler
    {
    public:
        [[nodiscard]] std::shared_ptr<PbrMaterialRuntime> Compile(const IMaterial& material,
            std::vector<std::string>& diagnostics) const;
    };
}
