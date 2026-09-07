#include "mesh_material/BuiltinTranslationLayers.h"

#include <cassert>

int main()
{
    using namespace mesh_material;
    MaterialFactory factory;
    const auto materialX = factory.Import({.format = MaterialFormat::MaterialX, .sourceIdentifier = "paint",
        .payload = {static_cast<std::byte>('<'), static_cast<std::byte>('m'), static_cast<std::byte>('/'), static_cast<std::byte>('>')}});
    assert(materialX.success && materialX.document.materialName == "paint");
    const auto material = factory.Create(5, materialX.document, {.name = "Paint"});
    assert(material && material->GetId() == 5 && material->GetRuntimePbr().id == 5);
    assert(factory.RegisterTranslationLayer(std::make_shared<GltfMaterialTranslationLayer>()));
    const auto gltf = factory.Import({.format = MaterialFormat::Gltf, .sourceIdentifier = "gltf-paint",
        .parameters = {{"baseColorFactor", Float4{.2f, .3f, .4f, 1.f}}, {"metallicFactor", .7f}, {"roughnessFactor", .4f}}});
    assert(gltf.success && gltf.document.xml.find("standard_surface") != std::string::npos);
    assert(factory.RegisterTranslationLayer(std::make_shared<UsdPreviewSurfaceTranslationLayer>()));
    const auto usd = factory.Import({.format = MaterialFormat::UsdShade, .sourceIdentifier = "usd-paint",
        .parameters = {{"diffuseColor", Float3{.2f, .3f, .4f}}, {"metallic", .7f}, {"roughness", .4f}}});
    assert(usd.success && usd.document.xml.find("base_color") != std::string::npos);
}
