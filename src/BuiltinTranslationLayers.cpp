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
        xml << "</standard_surface><surfacematerial name=\"material\" type=\"material\"><input name=\"surfaceshader\" type=\"surfaceshader\" nodename=\"surface\"/></surfacematerial></materialx>";
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
    std::string_view TextureSlotName(mesh_material::BakedTextureSlot slot)
    {
        using enum mesh_material::BakedTextureSlot;
        switch (slot)
        {
        case BaseColor: return "base_color";
        case Normal: return "normal";
        case Metallic: return "metalness";
        case Roughness: return "specular_roughness";
        case Occlusion: return "occlusion";
        case Emission: return "emission_color";
        case Opacity: return "opacity";
        }
        return "texture";
    }
    std::string_view TextureType(mesh_material::BakedTextureSlot slot)
    {
        using enum mesh_material::BakedTextureSlot;
        switch (slot)
        {
        case BaseColor: case Emission: return "color3";
        case Normal: return "vector3";
        default: return "float";
        }
    }
    std::string EscapeXml(std::string_view value)
    {
        std::string escaped;
        for (const char character : value)
        {
            switch (character)
            {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '\"': escaped += "&quot;"; break;
            case '\'': escaped += "&apos;"; break;
            default: escaped += character; break;
            }
        }
        return escaped;
    }
    mesh_material::PbrMaterialData ProjectPbr(const mesh_material::BakedMaterialSource& source, std::string_view name)
    {
        mesh_material::PbrMaterialData pbr{.name = std::string(name)};
        for (const auto& parameter : source.parameters)
        {
            if (parameter.name == "baseColor")
            {
                if (const auto* value = std::get_if<mesh_material::Float4>(&parameter.value)) pbr.baseColor = *value;
                else if (const auto* value = std::get_if<mesh_material::Float3>(&parameter.value)) pbr.baseColor = {value->x, value->y, value->z, 1.f};
            }
            else if (parameter.name == "metallic") { if (const auto* value = std::get_if<float>(&parameter.value)) pbr.metallic = *value; }
            else if (parameter.name == "roughness") { if (const auto* value = std::get_if<float>(&parameter.value)) pbr.roughness = *value; }
        }
        return pbr;
    }
    mesh_material::MaterialTranslationResult TranslateBaked(const mesh_material::MaterialTranslationRequest& request,
        mesh_material::BakedMaterialSourceKind expectedKind)
    {
        const auto& source = request.bakedSource;
        if (source.kind != expectedKind) return {.diagnostics = {"Baked material source kind does not match the translation layer"}};
        if (source.sourceUri.empty()) return {.diagnostics = {"Baked material source URI is empty"}};
        std::ostringstream xml;
        const auto sourceKind = source.kind == mesh_material::BakedMaterialSourceKind::SubstanceArchive ? "substance_archive" : "mdl_module";
        xml << "<?xml version=\"1.0\"?><materialx version=\"1.39\" sourceuri=\"" << EscapeXml(source.sourceUri)
            << "\" sourcekind=\"" << sourceKind << "\"";
        if (source.kind == mesh_material::BakedMaterialSourceKind::MdlModule) xml << " mdlmode=\"baked_distilled\"";
        xml << "><standard_surface name=\"surface\" type=\"surfaceshader\">";
        for (const auto& texture : source.textures)
        {
            if (texture.uri.empty()) continue;
            const auto slotName = TextureSlotName(texture.slot);
            xml << "<input name=\"" << slotName << "\" type=\"" << TextureType(texture.slot) << "\" nodename=\"" << slotName << "_image\"/>";
        }
        for (const auto& parameter : source.parameters)
        {
            if (parameter.name == "baseColor") AppendInput(xml, "base_color", &parameter);
            else if (parameter.name == "metallic") AppendInput(xml, "metalness", &parameter);
            else if (parameter.name == "roughness") AppendInput(xml, "specular_roughness", &parameter);
        }
        xml << "</standard_surface>";
        for (const auto& texture : source.textures)
        {
            if (texture.uri.empty()) continue;
            const auto slotName = TextureSlotName(texture.slot);
            xml << "<image name=\"" << slotName << "_image\" type=\"" << TextureType(texture.slot)
                << "\"><input name=\"file\" type=\"filename\" value=\"" << EscapeXml(texture.uri) << "\"/></image>";
        }
        xml << "<surfacematerial name=\"material\" type=\"material\"><input name=\"surfaceshader\" type=\"surfaceshader\" nodename=\"surface\"/></surfacematerial></materialx>";
        return {.success = true, .document = {.xml = xml.str(), .materialName = request.sourceIdentifier, .overrides = source.parameters},
            .runtimePbr = ProjectPbr(source, request.sourceIdentifier)};
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
    MaterialFormat SubstanceBakedMaterialTranslationLayer::GetFormat() const noexcept { return MaterialFormat::Substance; }
    MaterialTranslationResult SubstanceBakedMaterialTranslationLayer::Import(const MaterialTranslationRequest& request) const { return TranslateBaked(request, BakedMaterialSourceKind::SubstanceArchive); }
    bool SubstanceBakedMaterialTranslationLayer::Export(const MaterialXDocument& document, MaterialTranslationRequest& request, std::vector<std::string>& diagnostics) const { return ExportPbr(GetFormat(), document, request, diagnostics); }
    MaterialFormat MdlMaterialTranslationLayer::GetFormat() const noexcept { return MaterialFormat::Mdl; }
    MaterialTranslationResult MdlMaterialTranslationLayer::Import(const MaterialTranslationRequest& request) const
    {
        if (request.bakedSource.kind != BakedMaterialSourceKind::MdlModule) return {.diagnostics = {"MDL translation requires an MDL module baked source descriptor"}};
        if (request.bakedSource.mdlMode == MdlImportMode::Native)
            return {.diagnostics = {"Native MDL cannot be imported portably; a native renderer MDL adapter is required"}};
        return TranslateBaked(request, BakedMaterialSourceKind::MdlModule);
    }
    bool MdlMaterialTranslationLayer::Export(const MaterialXDocument& document, MaterialTranslationRequest& request, std::vector<std::string>& diagnostics) const { return ExportPbr(GetFormat(), document, request, diagnostics); }
}
