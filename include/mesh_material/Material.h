#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
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

    class IMaterial
    {
    public:
        virtual ~IMaterial() = default;
        [[nodiscard]] virtual AssetId GetId() const noexcept = 0;
        [[nodiscard]] virtual const MaterialXDocument& GetMaterialX() const noexcept = 0;
    };
}
