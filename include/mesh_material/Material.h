#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <memory>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace mesh_material
{
    using AssetId = std::uint64_t;
    inline constexpr AssetId InvalidAssetId = 0;

    struct Float2 { float x{}, y{}; };
    struct Float3 { float x{}, y{}, z{}; };
    struct Float4 { float x{}, y{}, z{}, w{}; };

    struct PbrMaterialData
    {
        AssetId id{};
        std::string name;
        Float4 baseColor{1.f, 1.f, 1.f, 1.f};
        float metallic{};
        float roughness{1.f};
        AssetId baseColorTexture{};
        AssetId normalTexture{};
        AssetId metallicRoughnessTexture{};
    };

    using ParameterValue = std::variant<float, Float2, Float3, Float4, std::int32_t, bool, std::string, AssetId>;

    struct Parameter
    {
        std::string name;
        ParameterValue value;
    };

    struct MaterialXDocument
    {
        std::string xml;
        std::string materialName;
        std::vector<Parameter> overrides;
    };

    enum class MaterialFormat : std::uint8_t
    {
        MaterialX, Gltf, UsdShade, ThreeDsMax, Mdl, Substance
    };

    struct MaterialTranslationRequest
    {
        MaterialFormat format{MaterialFormat::MaterialX};
        std::string sourceIdentifier;
        std::vector<std::byte> payload;
        std::vector<Parameter> parameters;
    };

    struct MaterialTranslationResult
    {
        bool success{};
        MaterialXDocument document;
        std::vector<std::string> diagnostics;
    };

    class IMaterial
    {
    public:
        virtual ~IMaterial() = default;
        [[nodiscard]] virtual AssetId GetId() const noexcept = 0;
        [[nodiscard]] virtual const MaterialXDocument& GetMaterialX() const noexcept = 0;
        [[nodiscard]] virtual const PbrMaterialData& GetRuntimePbr() const noexcept = 0;
    };

    class IMaterialTranslationLayer
    {
    public:
        virtual ~IMaterialTranslationLayer() = default;
        [[nodiscard]] virtual MaterialFormat GetFormat() const noexcept = 0;
        [[nodiscard]] virtual MaterialTranslationResult Import(const MaterialTranslationRequest& request) const = 0;
        [[nodiscard]] virtual bool Export(const MaterialXDocument& document, MaterialTranslationRequest& request,
            std::vector<std::string>& diagnostics) const = 0;
    };

    class IMaterialFactory
    {
    public:
        virtual ~IMaterialFactory() = default;
        virtual bool RegisterTranslationLayer(std::shared_ptr<IMaterialTranslationLayer> layer) = 0;
        [[nodiscard]] virtual std::shared_ptr<IMaterial> Create(AssetId id, MaterialXDocument document,
            PbrMaterialData runtimePbr = {}) const = 0;
        [[nodiscard]] virtual MaterialTranslationResult Import(const MaterialTranslationRequest& request) const = 0;
        [[nodiscard]] virtual bool Export(MaterialFormat format, const MaterialXDocument& document,
            MaterialTranslationRequest& request, std::vector<std::string>& diagnostics) const = 0;
    };

    class MaterialFactory final : public IMaterialFactory
    {
    public:
        bool RegisterTranslationLayer(std::shared_ptr<IMaterialTranslationLayer> layer) override;
        [[nodiscard]] std::shared_ptr<IMaterial> Create(AssetId id, MaterialXDocument document,
            PbrMaterialData runtimePbr = {}) const override;
        [[nodiscard]] MaterialTranslationResult Import(const MaterialTranslationRequest& request) const override;
        [[nodiscard]] bool Export(MaterialFormat format, const MaterialXDocument& document,
            MaterialTranslationRequest& request, std::vector<std::string>& diagnostics) const override;
    private:
        std::vector<std::shared_ptr<IMaterialTranslationLayer>> m_layers;
    };
}
