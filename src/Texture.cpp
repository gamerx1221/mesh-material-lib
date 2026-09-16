#include "mesh_material/Texture.h"

#include <cmath>
#include <set>
#include <sstream>
#include <tuple>

namespace
{
    std::size_t BytesPerPixel(mesh_material::PixelFormat format)
    {
        using enum mesh_material::PixelFormat;
        switch (format) { case R8Unorm: return 1; case RG8Unorm: return 2; case RGBA8Unorm: case RGBA8Srgb: return 4; case R16Float: return 2; case RG16Float: return 4; case RGBA16Float: return 8; case R32Float: return 4; case RGBA32Float: return 16; }
        return 0;
    }
    bool IsFinitePositive(float value) { return std::isfinite(value) && value > 0.f; }
}

namespace mesh_material
{
    TextureUri::TextureUri(std::string value) : m_value(std::move(value)) {}
    bool TextureUri::IsValid() const noexcept { return !m_value.empty() && m_value.find('\0') == std::string::npos; }
    TextureRequest::TextureRequest(TextureUri uri, TextureSemantic semantic, TextureColorSpace colorSpace, TextureRequestMetadata metadata)
        : m_uri(std::move(uri)), m_semantic(semantic), m_colorSpace(colorSpace), m_metadata(metadata) {}
    std::string TextureRequest::CacheKey() const
    {
        std::ostringstream key;
        key << m_uri.Value() << '|' << static_cast<int>(m_semantic) << '|' << static_cast<int>(m_colorSpace) << '|' << static_cast<int>(m_metadata.dimension) << '|' << static_cast<int>(m_metadata.uvSet) << '|'
            << m_metadata.transform.offsetU << ',' << m_metadata.transform.offsetV << ',' << m_metadata.transform.scaleU << ',' << m_metadata.transform.scaleV << ',' << m_metadata.transform.rotationRadians << '|'
            << static_cast<int>(m_metadata.sampler.addressU) << ',' << static_cast<int>(m_metadata.sampler.addressV) << ',' << static_cast<int>(m_metadata.sampler.addressW) << ',' << static_cast<int>(m_metadata.sampler.minFilter) << ',' << static_cast<int>(m_metadata.sampler.magFilter) << ',' << static_cast<int>(m_metadata.sampler.mipFilter) << ',' << m_metadata.sampler.maxAnisotropy << '|'
            << static_cast<int>(m_metadata.swizzle.red) << ',' << static_cast<int>(m_metadata.swizzle.green) << ',' << static_cast<int>(m_metadata.swizzle.blue) << ',' << static_cast<int>(m_metadata.swizzle.alpha);
        return key.str();
    }
    std::vector<TextureDiagnostic> TextureRequest::Validate() const
    {
        std::vector<TextureDiagnostic> diagnostics;
        if (!m_uri.IsValid()) diagnostics.push_back({TextureError::InvalidRequest, "Texture URI must be non-empty and contain no NUL character"});
        if (!IsFinitePositive(m_metadata.transform.scaleU) || !IsFinitePositive(m_metadata.transform.scaleV)) diagnostics.push_back({TextureError::InvalidRequest, "Texture transform scale must be finite and positive"});
        if (!IsFinitePositive(m_metadata.sampler.maxAnisotropy)) diagnostics.push_back({TextureError::InvalidRequest, "Sampler anisotropy must be finite and positive"});
        if (m_semantic == TextureSemantic::Normal && m_colorSpace != TextureColorSpace::Linear) diagnostics.push_back({TextureError::InvalidRequest, "Normal textures must use linear color space"});
        return diagnostics;
    }
    std::vector<TextureDiagnostic> DecodedTexture::Validate() const
    {
        std::vector<TextureDiagnostic> diagnostics;
        if (!width || !height || !depth || !arrayLayers || !mipCount) { diagnostics.push_back({TextureError::InvalidDecodedTexture, "Texture dimensions, array layers, and mip count must be non-zero"}); return diagnostics; }
        const std::uint32_t faces = dimension == ImageDimension::Cube || dimension == ImageDimension::CubeArray ? 6u : 1u;
        if ((dimension == ImageDimension::Cube || dimension == ImageDimension::CubeArray) && width != height) diagnostics.push_back({TextureError::InvalidDecodedTexture, "Cube textures must have square faces"});
        if (dimension == ImageDimension::Texture3D && arrayLayers != 1) diagnostics.push_back({TextureError::InvalidDecodedTexture, "3D textures cannot have array layers"});
        if (dimension != ImageDimension::Texture3D && depth != 1) diagnostics.push_back({TextureError::InvalidDecodedTexture, "Only 3D textures can have depth greater than one"});
        if (subresources.size() != static_cast<std::size_t>(mipCount) * arrayLayers * faces) diagnostics.push_back({TextureError::InvalidDecodedTexture, "Subresource count must cover every mip, layer, and cube face"});
        const auto bytesPerPixel = BytesPerPixel(format);
        std::set<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>> coordinates;
        for (const auto& level : subresources)
        {
            if (!level.bytes || !level.width || !level.height || !level.depth || level.mipLevel >= mipCount || level.arrayLayer >= arrayLayers || level.face >= faces) { diagnostics.push_back({TextureError::InvalidDecodedTexture, "Subresource metadata is invalid"}); continue; }
            if (!coordinates.emplace(level.mipLevel, level.arrayLayer, level.face).second) diagnostics.push_back({TextureError::InvalidDecodedTexture, "Subresource coordinates must be unique"});
            const auto expectedWidth = std::max(1u, width >> level.mipLevel); const auto expectedHeight = std::max(1u, height >> level.mipLevel); const auto expectedDepth = dimension == ImageDimension::Texture3D ? std::max(1u, depth >> level.mipLevel) : depth;
            if (level.width != expectedWidth || level.height != expectedHeight || level.depth != expectedDepth) diagnostics.push_back({TextureError::InvalidDecodedTexture, "Subresource dimensions do not match its mip level"});
            if (level.rowPitch < static_cast<std::size_t>(level.width) * bytesPerPixel || level.slicePitch < level.rowPitch * level.height || level.bytes->size() < level.slicePitch * level.depth) diagnostics.push_back({TextureError::InvalidDecodedTexture, "Subresource pitches or byte buffer are too small"});
        }
        return diagnostics;
    }
    TextureResult<TextureHandle> TexturePipeline::ResolveAndUpload(const TextureRequest& request)
    {
        if (const auto diagnostics = request.Validate(); !diagnostics.empty()) return {.diagnostics = diagnostics};
        auto decoded = m_resolver.Resolve(request);
        if (!decoded) return {.diagnostics = std::move(decoded.diagnostics)};
        if (const auto diagnostics = decoded.value->Validate(); !diagnostics.empty()) return {.diagnostics = diagnostics};
        auto uploaded = m_backend.Upload(request, *decoded.value);
        if (!uploaded && uploaded.diagnostics.empty()) return TextureResult<TextureHandle>::Failure(TextureError::UploadFailed, "Texture backend failed without a diagnostic");
        return uploaded;
    }
}
