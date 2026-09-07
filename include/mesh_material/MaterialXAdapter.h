#pragma once

#include "mesh_material/Material.h"

namespace mesh_material
{
    class MaterialXTranslationLayer final : public IMaterialTranslationLayer
    {
    public:
        [[nodiscard]] MaterialFormat GetFormat() const noexcept override;
        [[nodiscard]] MaterialTranslationResult Import(const MaterialTranslationRequest& request) const override;
        [[nodiscard]] bool Export(const MaterialXDocument& document, MaterialTranslationRequest& request,
            std::vector<std::string>& diagnostics) const override;
    };
}
