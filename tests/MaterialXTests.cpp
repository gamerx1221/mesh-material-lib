#include "mesh_material/MaterialXAdapter.h"
#include "mesh_material/MaterialXRuntime.h"
#include "mesh_material/BuiltinTranslationLayers.h"

#include <cassert>

int main()
{
    using namespace mesh_material;
    MaterialXTranslationLayer layer;
    const auto imported = layer.Import({.format = MaterialFormat::MaterialX, .sourceIdentifier = "empty",
        .payload = {static_cast<std::byte>('<'), static_cast<std::byte>('m'), static_cast<std::byte>('a'), static_cast<std::byte>('t'), static_cast<std::byte>('e'), static_cast<std::byte>('r'), static_cast<std::byte>('i'), static_cast<std::byte>('a'), static_cast<std::byte>('l'), static_cast<std::byte>('x'), static_cast<std::byte>('/'), static_cast<std::byte>('>')}});
    assert(imported.success);
    MaterialFactory factory;
    const auto material = factory.Create(7, {.xml = imported.document.xml, .materialName = "runtime", .overrides = {{"baseColorFactor", Float4{.2f, .3f, .4f, 1.f}}, {"metallicFactor", .6f}, {"emissiveFactor", Float3{.1f, .2f, .3f}}}});
    std::vector<std::string> diagnostics;
    const auto runtime = MaterialXRuntimeCompiler().Compile(*material, diagnostics);
    assert(runtime && runtime->document && runtime->pbr.metallic == .6f && runtime->emissive.z == .3f);
    SubstanceBakedMaterialTranslationLayer substanceLayer;
    const auto baked = substanceLayer.Import({.format = MaterialFormat::Substance, .sourceIdentifier = "paint",
        .bakedSource = {.kind = BakedMaterialSourceKind::SubstanceArchive, .sourceUri = "materials/paint.sbsar",
            .textures = {{BakedTextureSlot::BaseColor, "textures/paint_basecolor.png"}, {BakedTextureSlot::Normal, "textures/paint_normal.png"}}}});
    const auto validatedBaked = layer.Import({.format = MaterialFormat::MaterialX, .sourceIdentifier = "paint",
        .payload = std::vector<std::byte>(reinterpret_cast<const std::byte*>(baked.document.xml.data()),
            reinterpret_cast<const std::byte*>(baked.document.xml.data() + baked.document.xml.size()))});
    assert(baked.success && validatedBaked.success);
}
