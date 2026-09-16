# mesh-material-lib

Portable MaterialX-first material contracts for renderer and DCC integrations.

## Targets

- `mesh_material::core`: portable asset IDs, math/PBR types, material contracts, texture resolution/upload contracts, factory, and built-in glTF/USD Preview Surface, baked Substance, and baked MDL translators.
- `mesh_material::materialx`: optional MaterialX XML validation and fixed-PBR runtime compiler.
- Future optional targets: OpenEXR, KTX2, JPEG XR, and DDS/WIC adapters.

## Dependencies

Initialize recursively:

```powershell
git submodule update --init --recursive
```

Nested optional sources are MaterialX, OpenEXR, KTX-Software, jxrlib, DirectXTex, and MDL-SDK. Adobe Substance remains an external proprietary SDK path.

## Integration

```cmake
add_subdirectory(third_party/mesh-material-lib)
target_link_libraries(my_renderer PRIVATE mesh_material::core)
```

Enable `MESH_MATERIAL_ENABLE_MATERIALX=ON` to link `mesh_material::materialx`. The MaterialX source is owned by this repository's nested submodule, so consumers must not also add a separate MaterialX tree.

MaterialX is the authoritative material asset representation. Renderer adapters own GPU textures, descriptor bindings, shader compilation, and resource lifetime.

## Textures

`TextureRequest` is the immutable renderer-neutral input to texture loading. It contains the URI, PBR semantic, desired color space, image dimension, UV/transform, sampler, and channel swizzle. `ITextureAssetResolver` turns it into validated `DecodedTexture` subresources; codecs and URI schemes belong in resolver implementations. `ITextureUploadBackend` turns request plus decoded data into an opaque backend-owned `TextureHandle`; native DX12, Vulkan, Metal, and NVRHI code remains in backend adapters.

`TexturePipeline` is the minimal resolve-then-upload helper. Cache resolvers by `TextureRequest::CacheKey()`, not URI alone. `DecodedTexture` owns byte data through immutable shared buffers so adapters may safely use pinned zero-copy payloads. See `adapter_instructions.md` for required backend behavior.

## Baked Substance and MDL

`BakedMaterialSource` is the SDK-independent import descriptor for Substance archives and MDL modules. It records the original URI, baked PBR texture URIs, and scalar/color PBR parameters. Register `SubstanceBakedMaterialTranslationLayer` or `MdlMaterialTranslationLayer` with `MaterialFactory`, then import using `MaterialFormat::Substance` or `MaterialFormat::Mdl`.

Both baked paths produce the same portable artifact: a MaterialX `standard_surface` graph with `image` nodes referencing the supplied URIs, origin metadata on the MaterialX root, and a projected `PbrMaterialData` in `MaterialTranslationResult`. `MaterialFactory::Create(id, result)` preserves that projection.

MDL has two explicit modes. `MdlImportMode::BakedDistilled` accepts baked PBR maps. `MdlImportMode::Native` deliberately returns an unsuccessful result with a diagnostic requiring a native renderer MDL adapter; the core library never compiles or approximates native MDL.
