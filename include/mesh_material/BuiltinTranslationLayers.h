#pragma once

#include "mesh_material/Material.h"

namespace mesh_material
{
    class GltfMaterialTranslationLayer final : public IMaterialTranslationLayer
    {
    public:
        [[nodiscard]] MaterialFormat GetFormat() const noexcept override;
        [[nodiscard]] MaterialTranslationResult Import(const MaterialTranslationRequest& request) const override;
        [[nodiscard]] bool Export(const MaterialXDocument& document, MaterialTranslationRequest& request,
            std::vector<std::string>& diagnostics) const override;
    };

    class UsdPreviewSurfaceTranslationLayer final : public IMaterialTranslationLayer
    {
    public:
        [[nodiscard]] MaterialFormat GetFormat() const noexcept override;
        [[nodiscard]] MaterialTranslationResult Import(const MaterialTranslationRequest& request) const override;
        [[nodiscard]] bool Export(const MaterialXDocument& document, MaterialTranslationRequest& request,
            std::vector<std::string>& diagnostics) const override;
    };

    /// Imports PBR maps baked from a Substance archive without linking the Adobe SDK.
    class SubstanceBakedMaterialTranslationLayer final : public IMaterialTranslationLayer
    {
    public:
        [[nodiscard]] MaterialFormat GetFormat() const noexcept override;
        [[nodiscard]] MaterialTranslationResult Import(const MaterialTranslationRequest& request) const override;
        [[nodiscard]] bool Export(const MaterialXDocument& document, MaterialTranslationRequest& request,
            std::vector<std::string>& diagnostics) const override;
    };

    /// Imports distilled/baked MDL PBR maps. Native MDL requires a renderer-specific adapter and is rejected.
    class MdlMaterialTranslationLayer final : public IMaterialTranslationLayer
    {
    public:
        [[nodiscard]] MaterialFormat GetFormat() const noexcept override;
        [[nodiscard]] MaterialTranslationResult Import(const MaterialTranslationRequest& request) const override;
        [[nodiscard]] bool Export(const MaterialXDocument& document, MaterialTranslationRequest& request,
            std::vector<std::string>& diagnostics) const override;
    };
}
