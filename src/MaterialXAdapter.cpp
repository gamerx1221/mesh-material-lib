#include "mesh_material/MaterialXAdapter.h"

#include <MaterialXFormat/XmlIo.h>

namespace mesh_material
{
    MaterialFormat MaterialXTranslationLayer::GetFormat() const noexcept { return MaterialFormat::MaterialX; }
    MaterialTranslationResult MaterialXTranslationLayer::Import(const MaterialTranslationRequest& request) const
    {
        MaterialTranslationResult result;
        if (request.payload.empty()) { result.diagnostics.push_back("MaterialX payload is empty"); return result; }
        try
        {
            const std::string xml(reinterpret_cast<const char*>(request.payload.data()), request.payload.size());
            const auto document = MaterialX::createDocument();
            MaterialX::readFromXmlString(document, xml);
            std::string validationMessage;
            if (!document->validate(&validationMessage)) { result.diagnostics.push_back(validationMessage); return result; }
            result.success = true;
            result.document = {.xml = MaterialX::writeToXmlString(document), .materialName = request.sourceIdentifier, .overrides = request.parameters};
        }
        catch (const MaterialX::Exception& exception) { result.diagnostics.push_back(exception.what()); }
        return result;
    }
    bool MaterialXTranslationLayer::Export(const MaterialXDocument& document, MaterialTranslationRequest& request,
        std::vector<std::string>& diagnostics) const
    {
        MaterialTranslationRequest validationRequest{.format = MaterialFormat::MaterialX, .sourceIdentifier = document.materialName,
            .payload = std::vector<std::byte>(reinterpret_cast<const std::byte*>(document.xml.data()), reinterpret_cast<const std::byte*>(document.xml.data() + document.xml.size())), .parameters = document.overrides};
        const auto validated = Import(validationRequest);
        if (!validated.success) { diagnostics.insert(diagnostics.end(), validated.diagnostics.begin(), validated.diagnostics.end()); return false; }
        request = std::move(validationRequest);
        return true;
    }
}
