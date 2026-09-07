#include "mesh_material/BuiltinTranslationLayers.h"

#include <sstream>

namespace
{
    const mesh_material::Parameter* FindParameter(const std::vector<mesh_material::Parameter>& parameters, std::string_view name)
    {
        const auto found = std::ranges::find_if(parameters, [name](const auto& parameter) { return parameter.name == name; });
        return found == parameters.end() ? nullptr : &*found;
    }
    void AppendInput(std::ostringstream& xml, std::string_view name, const mesh_material::Parameter* parameter)
    {
        if (!parameter) return;
        if (const auto* value = std::get_if<float>(&parameter->value)) xml << "<input name=\"" << name << "\" type=\"float\" value=\"" << *value << "\"/>";
        else if (const auto* value = std::get_if<mesh_material::Float3>(&parameter->value)) xml << "<input name=\"" << name << "\" type=\"color3\" value=\"" << value->x << ", " << value->y << ", " << value->z << "\"/>";
        else if (const auto* value = std::get_if<mesh_material::Float4>(&parameter->value)) xml << "<input name=\"" << name << "\" type=\"color4\" value=\"" << value->x << ", " << value->y << ", " << value->z << ", " << value->w << "\"/>";
    }
    mesh_material::MaterialTranslationResult TranslatePbr(const mesh_material::MaterialTranslationRequest& request, std::string_view color,
        std::string_view metallic, std::string_view roughness, std::string_view emissive, std::string_view opacity)
    {
        std::ostringstream xml;
        xml << "<?xml version=\"1.0\"?><materialx version=\"1.39\"><standard_surface name=\"surface\" type=\"surfaceshader\">";
        AppendInput(xml, "base_color", FindParameter(request.parameters, color));
        AppendInput(xml, "metalness", FindParameter(request.parameters, metallic));
        AppendInput(xml, "specular_roughness", FindParameter(request.parameters, roughness));
        AppendInput(xml, "emission_color", FindParameter(request.parameters, emissive));
        AppendInput(xml, "opacity", FindParameter(request.parameters, opacity));
        xml << "</standard_surface><surfacematerial name=\"material\"><input name=\"surfaceshader\" type=\"surfaceshader\" nodename=\"surface\"/></surfacematerial></materialx>";
        return {.success = true, .document = {.xml = xml.str(), .materialName = request.sourceIdentifier, .overrides = request.parameters}};
    }
    bool ExportPbr(mesh_material::MaterialFormat format, const mesh_material::MaterialXDocument& document,
        mesh_material::MaterialTranslationRequest& request, std::vector<std::string>& diagnostics)
    {
        if (document.xml.empty()) { diagnostics.push_back("MaterialX document is empty"); return false; }
        request.format = format; request.sourceIdentifier = document.materialName; request.parameters = document.overrides;
        diagnostics.push_back("Only supported PBR parameter overrides are exported; arbitrary MaterialX graphs are preserved as MaterialX.");
        return true;
    }
}
namespace mesh_material
{
    MaterialFormat GltfMaterialTranslationLayer::GetFormat() const noexcept { return MaterialFormat::Gltf; }
    MaterialTranslationResult GltfMaterialTranslationLayer::Import(const MaterialTranslationRequest& request) const { return TranslatePbr(request, "baseColorFactor", "metallicFactor", "roughnessFactor", "emissiveFactor", "alpha"); }
    bool GltfMaterialTranslationLayer::Export(const MaterialXDocument& document, MaterialTranslationRequest& request, std::vector<std::string>& diagnostics) const { return ExportPbr(GetFormat(), document, request, diagnostics); }
    MaterialFormat UsdPreviewSurfaceTranslationLayer::GetFormat() const noexcept { return MaterialFormat::UsdShade; }
    MaterialTranslationResult UsdPreviewSurfaceTranslationLayer::Import(const MaterialTranslationRequest& request) const { return TranslatePbr(request, "diffuseColor", "metallic", "roughness", "emissiveColor", "opacity"); }
    bool UsdPreviewSurfaceTranslationLayer::Export(const MaterialXDocument& document, MaterialTranslationRequest& request, std::vector<std::string>& diagnostics) const { return ExportPbr(GetFormat(), document, request, diagnostics); }
}
