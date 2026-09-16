# Texture Adapter Instructions

Implement adapters outside `mesh_material::core`. This repository defines contracts only: no native graphics headers, upload commands, codecs, or descriptor allocation belong in shared code.

## Required Flow

1. Convert every material texture input to a `TextureRequest`. Preserve URI, semantic, requested color space, dimension, UV set, transform, sampler, and swizzle.
2. Implement `ITextureAssetResolver` using the application asset system and codecs. Validate source data, normalize all output into `DecodedTexture` mip/layer/face subresources, and return structured diagnostics.
3. Implement `ITextureUploadBackend` in each renderer adapter. It must validate capabilities, create the native image/resource, upload every subresource using its row/slice pitches, create the sampled descriptor/view, and return a `TextureResource` with a stable identity.
4. Call `Release` on the adapter's documented graphics thread after GPU completion. The resource object is backend-owned; never cast it in shared code.

## Backend Steps

DX12: create a default-heap `ID3D12Resource`, stage with an upload buffer and `CopyTextureRegion`, transition to shader-resource state, allocate an SRV descriptor, fence the copy, and defer resource/descriptor destruction until that fence completes.

Vulkan: create `VkImage` plus memory, stage through a buffer and `vkCmdCopyBufferToImage` for every mip/layer/face, issue layout/access barriers to shader-read, create `VkImageView` and `VkSampler`, and retire all objects after the submission timeline value.

Metal: create `MTLTexture`, use a blit command encoder or `replaceRegion` only where permitted, make an `MTLSamplerState`, synchronize managed storage on macOS when required, and release after the command buffer completes.

NVRHI: create `nvrhi::Texture` and `nvrhi::Sampler` in the NVRHI adapter, write every subresource through its command-list upload/write API, set resource states before sampling, retain NVRHI handles with the adapter resource, and retire only after the command list fence completes.

## Color And Material Rules

Base color and emissive normally request sRGB. Normal, metallic-roughness, occlusion, height, opacity, and generic data are linear. Never sRGB-decode normal maps; preserve the authored tangent-space convention and apply normal scale in material shading. Swizzles describe source interpretation, not a reason to silently alter pixels. Reject incompatible requests rather than guessing.

## Cache, Errors, And Tests

Use the complete `TextureRequest::CacheKey()` for decoded-data caches. GPU caches additionally include backend/device identity and descriptor/view policy. Resolver results must outlive the resolver call; shared immutable byte buffers may pin mapped/container memory. Do not cache failures indefinitely unless the asset system version-stamps them.

Return diagnostics for missing assets, malformed images, unsupported format/dimension, capability mismatch, submission failure, and thread-affinity violations. Fallback textures are an application/material-adapter policy: use documented white sRGB base color, flat linear normal, white linear occlusion/metallic-roughness, and black sRGB emissive only when the caller explicitly chooses fallback.

Test request validation and cache-key distinctions; invalid dimensions, pitches, and subresource counts; all mip/layer/cube upload paths; sRGB and normal-map behavior; descriptor/resource identity; cache eviction; synchronization and deferred release; device loss; and fallback policy. Native resource code must live in backend adapters.
