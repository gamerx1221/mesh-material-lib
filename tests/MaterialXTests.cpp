#include "mesh_material/MaterialXAdapter.h"
#include "mesh_material/MaterialXRuntime.h"

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
}
