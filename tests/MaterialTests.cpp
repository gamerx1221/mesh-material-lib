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
    assert(factory.RegisterTranslationLayer(std::make_shared<SubstanceBakedMaterialTranslationLayer>()));
    const BakedMaterialSource bakedSubstance{.kind = BakedMaterialSourceKind::SubstanceArchive, .sourceUri = "materials/paint.sbsar",
        .textures = {{BakedTextureSlot::BaseColor, "textures/paint_basecolor.png"}, {BakedTextureSlot::Normal, "textures/paint_normal.png"}},
        .parameters = {{"baseColor", Float4{.2f, .3f, .4f, 1.f}}, {"metallic", .7f}, {"roughness", .4f}}};
    const auto substance = factory.Import({.format = MaterialFormat::Substance, .sourceIdentifier = "substance-paint", .bakedSource = bakedSubstance});
    assert(substance.success && substance.document.xml.find("sourcekind=\"substance_archive\"") != std::string::npos);
    assert(substance.runtimePbr.metallic == .7f && substance.runtimePbr.roughness == .4f);
    assert(factory.Create(9, substance)->GetRuntimePbr().name == "substance-paint");
    assert(factory.RegisterTranslationLayer(std::make_shared<MdlMaterialTranslationLayer>()));
    const auto nativeMdl = factory.Import({.format = MaterialFormat::Mdl, .sourceIdentifier = "native-paint",
        .bakedSource = {.kind = BakedMaterialSourceKind::MdlModule, .sourceUri = "materials/paint.mdl", .mdlMode = MdlImportMode::Native}});
    assert(!nativeMdl.success && nativeMdl.diagnostics.front().find("native renderer MDL adapter") != std::string::npos);
    const auto bakedMdl = factory.Import({.format = MaterialFormat::Mdl, .sourceIdentifier = "mdl-paint",
        .bakedSource = {.kind = BakedMaterialSourceKind::MdlModule, .sourceUri = "materials/paint.mdl", .mdlMode = MdlImportMode::BakedDistilled,
            .textures = bakedSubstance.textures, .parameters = bakedSubstance.parameters}});
    assert(bakedMdl.success && bakedMdl.document.xml.find("mdlmode=\"baked_distilled\"") != std::string::npos);
}
