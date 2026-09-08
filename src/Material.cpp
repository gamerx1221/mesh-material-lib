#include "mesh_material/Material.h"

#include <algorithm>

namespace
{
    class Material final : public mesh_material::IMaterial
    {
    public:
        Material(mesh_material::AssetId id, mesh_material::MaterialXDocument document, mesh_material::PbrMaterialData runtimePbr)
            : m_id(id), m_document(std::move(document)), m_runtimePbr(std::move(runtimePbr)) {}
        [[nodiscard]] mesh_material::AssetId GetId() const noexcept override { return m_id; }
        [[nodiscard]] const mesh_material::MaterialXDocument& GetMaterialX() const noexcept override { return m_document; }
        [[nodiscard]] const mesh_material::PbrMaterialData& GetRuntimePbr() const noexcept override { return m_runtimePbr; }
    private:
        mesh_material::AssetId m_id{};
        mesh_material::MaterialXDocument m_document;
        mesh_material::PbrMaterialData m_runtimePbr;
    };
}

namespace mesh_material
{
    bool MaterialFactory::RegisterTranslationLayer(std::shared_ptr<IMaterialTranslationLayer> layer)
    {
        if (!layer || std::ranges::any_of(m_layers, [&layer](const auto& existing) { return existing->GetFormat() == layer->GetFormat(); })) return false;
        m_layers.push_back(std::move(layer));
        return true;
    }
    std::shared_ptr<IMaterial> MaterialFactory::Create(AssetId id, MaterialXDocument document, PbrMaterialData runtimePbr) const
    {
        if (id == InvalidAssetId || document.xml.empty()) return {};
        runtimePbr.id = id;
        return std::make_shared<Material>(id, std::move(document), std::move(runtimePbr));
    }
    std::shared_ptr<IMaterial> MaterialFactory::Create(AssetId id, const MaterialTranslationResult& imported) const
    {
        if (!imported.success) return {};
        return Create(id, imported.document, imported.runtimePbr);
    }
    MaterialTranslationResult MaterialFactory::Import(const MaterialTranslationRequest& request) const
    {
        const auto layer = std::ranges::find_if(m_layers, [&request](const auto& candidate) { return candidate->GetFormat() == request.format; });
        if (layer != m_layers.end()) return (*layer)->Import(request);
        if (request.format == MaterialFormat::MaterialX)
        {
            MaterialTranslationResult result;
            result.document.xml.assign(reinterpret_cast<const char*>(request.payload.data()), request.payload.size());
            result.document.materialName = request.sourceIdentifier;
            result.document.overrides = request.parameters;
            result.success = !result.document.xml.empty();
            if (!result.success) result.diagnostics.push_back("MaterialX payload is empty");
            return result;
        }
        return {.diagnostics = {"No translation layer is registered for the source material format"}};
    }
    bool MaterialFactory::Export(MaterialFormat format, const MaterialXDocument& document, MaterialTranslationRequest& request,
        std::vector<std::string>& diagnostics) const
    {
        if (document.xml.empty()) { diagnostics.push_back("MaterialX document is empty"); return false; }
        const auto layer = std::ranges::find_if(m_layers, [format](const auto& candidate) { return candidate->GetFormat() == format; });
        if (layer != m_layers.end()) return (*layer)->Export(document, request, diagnostics);
        if (format == MaterialFormat::MaterialX)
        {
            request.format = format;
            request.sourceIdentifier = document.materialName;
            request.payload.assign(reinterpret_cast<const std::byte*>(document.xml.data()), reinterpret_cast<const std::byte*>(document.xml.data() + document.xml.size()));
            request.parameters = document.overrides;
            return true;
        }
        diagnostics.push_back("No translation layer is registered for the target material format");
        return false;
    }
}
