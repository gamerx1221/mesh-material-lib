#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mesh_material
{
    enum class TextureSemantic : std::uint8_t { BaseColor, MetallicRoughness, Normal, Occlusion, Emissive, Opacity, Height, Environment, Data };
    enum class TextureColorSpace : std::uint8_t { Linear, Srgb, HdrLinear, Unspecified };
    enum class ImageDimension : std::uint8_t { Texture2D, Texture3D, Cube, Texture2DArray, CubeArray };
    enum class PixelFormat : std::uint8_t { R8Unorm, RG8Unorm, RGBA8Unorm, RGBA8Srgb, R16Float, RG16Float, RGBA16Float, R32Float, RGBA32Float };
    enum class AlphaMode : std::uint8_t { Opaque, Straight, Premultiplied, Unknown };
    enum class AddressMode : std::uint8_t { Repeat, MirroredRepeat, ClampToEdge, ClampToBorder };
    enum class FilterMode : std::uint8_t { Nearest, Linear };
    enum class MipFilterMode : std::uint8_t { None, Nearest, Linear };
    enum class Channel : std::uint8_t { Red, Green, Blue, Alpha, Zero, One };
    enum class TextureError : std::uint8_t { InvalidRequest, InvalidDecodedTexture, NotFound, Unsupported, DecodeFailed, UploadFailed, ThreadAffinity };

    struct TextureDiagnostic { TextureError code{}; std::string message; };

    template<class T>
    struct TextureResult
    {
        std::optional<T> value;
        std::vector<TextureDiagnostic> diagnostics;
        [[nodiscard]] explicit operator bool() const noexcept { return value.has_value(); }
        [[nodiscard]] static TextureResult Success(T result) { return {.value = std::move(result)}; }
        [[nodiscard]] static TextureResult Failure(TextureError code, std::string message) { return {.diagnostics = {{code, std::move(message)}}}; }
    };

    class TextureUri
    {
    public:
        explicit TextureUri(std::string value = {});
        [[nodiscard]] const std::string& Value() const noexcept { return m_value; }
        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] friend bool operator==(const TextureUri&, const TextureUri&) = default;
    private:
        std::string m_value;
    };

    struct TextureTransform { float offsetU{}, offsetV{}; float scaleU{1.f}, scaleV{1.f}; float rotationRadians{}; };
    struct TextureSampler { AddressMode addressU{AddressMode::Repeat}; AddressMode addressV{AddressMode::Repeat}; AddressMode addressW{AddressMode::Repeat}; FilterMode minFilter{FilterMode::Linear}; FilterMode magFilter{FilterMode::Linear}; MipFilterMode mipFilter{MipFilterMode::Linear}; float maxAnisotropy{1.f}; };
    struct ChannelSwizzle { Channel red{Channel::Red}; Channel green{Channel::Green}; Channel blue{Channel::Blue}; Channel alpha{Channel::Alpha}; };
    struct TextureRequestMetadata { std::uint8_t uvSet{}; TextureTransform transform{}; TextureSampler sampler{}; ChannelSwizzle swizzle{}; ImageDimension dimension{ImageDimension::Texture2D}; };

    class TextureRequest
    {
    public:
        TextureRequest(TextureUri uri, TextureSemantic semantic, TextureColorSpace colorSpace, TextureRequestMetadata metadata = {});
        [[nodiscard]] const TextureUri& Uri() const noexcept { return m_uri; }
        [[nodiscard]] TextureSemantic Semantic() const noexcept { return m_semantic; }
        [[nodiscard]] TextureColorSpace ColorSpace() const noexcept { return m_colorSpace; }
        [[nodiscard]] const TextureRequestMetadata& Metadata() const noexcept { return m_metadata; }
        [[nodiscard]] std::string CacheKey() const;
        [[nodiscard]] std::vector<TextureDiagnostic> Validate() const;
        [[nodiscard]] friend bool operator==(const TextureRequest&, const TextureRequest&) = default;
    private:
        TextureUri m_uri;
        TextureSemantic m_semantic;
        TextureColorSpace m_colorSpace;
        TextureRequestMetadata m_metadata;
    };

    struct TextureMipLevel
    {
        std::uint32_t mipLevel{};
        std::uint32_t arrayLayer{};
        std::uint32_t face{};
        std::uint32_t width{};
        std::uint32_t height{};
        std::uint32_t depth{};
        std::size_t rowPitch{};
        std::size_t slicePitch{};
        // Immutable shared storage permits a decoder to pin a zero-copy payload safely.
        std::shared_ptr<const std::vector<std::byte>> bytes;
    };

    struct DecodedTexture
    {
        ImageDimension dimension{ImageDimension::Texture2D};
        PixelFormat format{PixelFormat::RGBA8Unorm};
        TextureColorSpace colorSpace{TextureColorSpace::Linear};
        AlphaMode alphaMode{AlphaMode::Unknown};
        std::uint32_t width{};
        std::uint32_t height{1};
        std::uint32_t depth{1};
        std::uint32_t arrayLayers{1};
        std::uint32_t mipCount{1};
        ChannelSwizzle swizzle{};
        std::vector<TextureMipLevel> subresources;
        [[nodiscard]] std::vector<TextureDiagnostic> Validate() const;
    };

    class ITextureAssetResolver
    {
    public:
        virtual ~ITextureAssetResolver() = default;
        // Implementations may cache by TextureRequest::CacheKey and resolve concurrently. Returned data must remain valid independently of the resolver.
        [[nodiscard]] virtual TextureResult<DecodedTexture> Resolve(const TextureRequest& request) = 0;
    };

    struct TextureBackendCapabilities { bool supportsTexture3D{}; bool supportsCube{}; bool supportsArrays{}; bool supportsSrgb{}; bool requiresRenderThread{}; };
    class TextureResource
    {
    public:
        virtual ~TextureResource() = default;
        [[nodiscard]] virtual std::uint64_t StableIdentity() const noexcept = 0;
    };
    using TextureHandle = std::shared_ptr<TextureResource>;

    class ITextureUploadBackend
    {
    public:
        virtual ~ITextureUploadBackend() = default;
        [[nodiscard]] virtual TextureBackendCapabilities Capabilities() const noexcept = 0;
        // Upload creates backend-owned native resources and descriptors. The caller must call Release on the backend's required thread before dropping the final handle.
        [[nodiscard]] virtual TextureResult<TextureHandle> Upload(const TextureRequest& request, const DecodedTexture& texture) = 0;
        virtual void Release(TextureHandle handle) = 0;
    };

    class TexturePipeline
    {
    public:
        TexturePipeline(ITextureAssetResolver& resolver, ITextureUploadBackend& backend) : m_resolver(resolver), m_backend(backend) {}
        [[nodiscard]] TextureResult<TextureHandle> ResolveAndUpload(const TextureRequest& request);
    private:
        ITextureAssetResolver& m_resolver;
        ITextureUploadBackend& m_backend;
    };
}
