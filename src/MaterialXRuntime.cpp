#include "mesh_material/MaterialXRuntime.h"

#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>

#include <mutex>
#include <sstream>

namespace
{
    const mesh_material::Parameter* FindParameter(const std::vector<mesh_material::Parameter>& parameters, std::initializer_list<std::string_view> names)
    {
        for (auto name : names) for (const auto& parameter : parameters) if (parameter.name == name) return &parameter;
        return nullptr;
    }
    void AssignFloat(const mesh_material::Parameter* parameter, float& destination) { if (parameter) if (const auto* value = std::get_if<float>(&parameter->value)) destination = *value; }
    void AssignColor(const mesh_material::Parameter* parameter, mesh_material::Float4& destination)
    {
        if (!parameter) return;
        if (const auto* value = std::get_if<mesh_material::Float4>(&parameter->value)) destination = *value;
        else if (const auto* value = std::get_if<mesh_material::Float3>(&parameter->value)) destination = {value->x, value->y, value->z, destination.w};
    }
    void AssignColor(const mesh_material::Parameter* parameter, mesh_material::Float3& destination)
    {
        if (!parameter) return;
        if (const auto* value = std::get_if<mesh_material::Float4>(&parameter->value)) destination = {value->x, value->y, value->z};
        else if (const auto* value = std::get_if<mesh_material::Float3>(&parameter->value)) destination = *value;
    }
    void AssignAsset(const mesh_material::Parameter* parameter, mesh_material::AssetId& destination) { if (parameter) if (const auto* value = std::get_if<mesh_material::AssetId>(&parameter->value)) destination = *value; }
    MaterialX::DocumentPtr GetStandardLibrary()
    {
        static std::once_flag initialized; static MaterialX::DocumentPtr library;
        std::call_once(initialized, [] { library = MaterialX::createDocument(); MaterialX::loadLibraries({"stdlib", "pbrlib", "bxdf", "lights"}, MaterialX::FileSearchPath(MESH_MATERIAL_MATERIALX_LIBRARY_ROOT), library); });
        return library;
    }
    std::vector<float> ParseValues(std::string values)
    {
        for (char& character : values) if (character == ',') character = ' ';
        std::istringstream stream(values); std::vector<float> result; float value{}; while (stream >> value) result.push_back(value); return result;
    }
    void ReadFloat(const MaterialX::NodePtr& node, std::string_view name, float& value)
    {
        const auto input = node->getInput(std::string(name)); if (!input || !input->hasValueString()) return;
        const auto values = ParseValues(input->getValueString()); if (!values.empty()) value = values[0];
    }
    void ReadColor(const MaterialX::NodePtr& node, std::string_view name, mesh_material::Float4& value)
    {
        const auto input = node->getInput(std::string(name)); if (!input || !input->hasValueString()) return;
        const auto values = ParseValues(input->getValueString()); if (values.size() >= 3) value = {values[0], values[1], values[2], value.w};
    }
    void ReadColor(const MaterialX::NodePtr& node, std::string_view name, mesh_material::Float3& value) { mesh_material::Float4 color{value.x, value.y, value.z, 1.f}; ReadColor(node, name, color); value = {color.x, color.y, color.z}; }
}
namespace mesh_material
{
    std::shared_ptr<PbrMaterialRuntime> MaterialXRuntimeCompiler::Compile(const IMaterial& material, std::vector<std::string>& diagnostics) const
    {
        const auto& source = material.GetMaterialX(); if (source.xml.empty()) { diagnostics.push_back("MaterialX document is empty"); return {}; }
        try
        {
            auto runtime = std::make_shared<PbrMaterialRuntime>(); runtime->document = MaterialX::createDocument(); runtime->document->setDataLibrary(GetStandardLibrary()); MaterialX::readFromXmlString(runtime->document, source.xml);
            std::string validationMessage; if (!runtime->document->validate(&validationMessage)) { diagnostics.push_back(validationMessage); return {}; }
            runtime->pbr = material.GetRuntimePbr(); runtime->pbr.id = material.GetId();
            const auto surfaces = runtime->document->getNodes("standard_surface");
            if (!surfaces.empty()) { ReadColor(surfaces.front(), "base_color", runtime->pbr.baseColor); ReadFloat(surfaces.front(), "metalness", runtime->pbr.metallic); ReadFloat(surfaces.front(), "specular_roughness", runtime->pbr.roughness); ReadColor(surfaces.front(), "emission_color", runtime->emissive); }
            else diagnostics.push_back("No standard_surface node found; using the runtime PBR projection and explicit overrides");
            AssignColor(FindParameter(source.overrides, {"baseColorFactor", "diffuseColor", "base_color"}), runtime->pbr.baseColor);
            AssignFloat(FindParameter(source.overrides, {"metallicFactor", "metallic", "metalness"}), runtime->pbr.metallic);
            AssignFloat(FindParameter(source.overrides, {"roughnessFactor", "roughness", "specular_roughness"}), runtime->pbr.roughness);
            AssignColor(FindParameter(source.overrides, {"emissiveFactor", "emissiveColor", "emission_color"}), runtime->emissive);
            AssignFloat(FindParameter(source.overrides, {"normalScale", "normal_scale"}), runtime->normalScale);
            AssignFloat(FindParameter(source.overrides, {"occlusionStrength", "occlusion_strength"}), runtime->occlusionStrength);
            AssignAsset(FindParameter(source.overrides, {"baseColorTexture"}), runtime->textures[0].asset); AssignAsset(FindParameter(source.overrides, {"metallicRoughnessTexture"}), runtime->textures[1].asset); AssignAsset(FindParameter(source.overrides, {"normalTexture"}), runtime->textures[2].asset); AssignAsset(FindParameter(source.overrides, {"occlusionTexture"}), runtime->textures[3].asset); AssignAsset(FindParameter(source.overrides, {"emissiveTexture"}), runtime->textures[4].asset);
            return runtime;
        }
        catch (const MaterialX::Exception& exception) { diagnostics.push_back(exception.what()); return {}; }
    }
}
