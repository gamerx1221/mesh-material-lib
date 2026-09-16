#include "mesh_material/Texture.h"

#include <cassert>

namespace
{
    using namespace mesh_material;
    DecodedTexture ValidTexture()
    {
        auto bytes = std::make_shared<const std::vector<std::byte>>(16);
        return {.width = 2, .height = 2, .subresources = {{.width = 2, .height = 2, .depth = 1, .rowPitch = 8, .slicePitch = 16, .bytes = std::move(bytes)}}};
    }
    class Resolver final : public ITextureAssetResolver
    {
    public:
        bool fail{};
        TextureResult<DecodedTexture> Resolve(const TextureRequest&) override { return fail ? TextureResult<DecodedTexture>::Failure(TextureError::NotFound, "missing") : TextureResult<DecodedTexture>::Success(ValidTexture()); }
    };
    class Resource final : public TextureResource { public: std::uint64_t StableIdentity() const noexcept override { return 42; } };
    class Backend final : public ITextureUploadBackend
    {
    public:
        bool fail{};
        TextureBackendCapabilities Capabilities() const noexcept override { return {}; }
        TextureResult<TextureHandle> Upload(const TextureRequest&, const DecodedTexture&) override { return fail ? TextureResult<TextureHandle>::Failure(TextureError::UploadFailed, "upload") : TextureResult<TextureHandle>::Success(std::make_shared<Resource>()); }
        void Release(TextureHandle) override {}
    };
}

void RunTextureTests()
{
    using namespace mesh_material;
    const TextureRequest base{TextureUri{"textures/paint.png"}, TextureSemantic::BaseColor, TextureColorSpace::Srgb};
    assert(base.Validate().empty() && base.CacheKey() == base.CacheKey());
    const TextureRequest different{TextureUri{"textures/paint.png"}, TextureSemantic::Normal, TextureColorSpace::Linear};
    assert(base.CacheKey() != different.CacheKey());
    const TextureRequest invalidRequest{TextureUri{""}, TextureSemantic::Normal, TextureColorSpace::Srgb};
    assert(!invalidRequest.Validate().empty());
    assert(ValidTexture().Validate().empty());
    auto invalid = ValidTexture(); invalid.subresources[0].rowPitch = 1; assert(!invalid.Validate().empty());
    invalid = ValidTexture(); invalid.subresources.push_back(invalid.subresources.front()); assert(!invalid.Validate().empty());
    Resolver resolver; Backend backend; TexturePipeline pipeline(resolver, backend);
    const auto success = pipeline.ResolveAndUpload(base); assert(success && (*success.value)->StableIdentity() == 42);
    resolver.fail = true; assert(!pipeline.ResolveAndUpload(base));
    resolver.fail = false; backend.fail = true; assert(!pipeline.ResolveAndUpload(base));
}
