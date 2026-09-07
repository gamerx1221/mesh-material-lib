# mesh-material-lib

Portable MaterialX-first material contracts for renderer and DCC integrations.

## Targets

- `mesh_material::core`: backend-neutral material interfaces and texture metadata.
- Future optional targets: MaterialX runtime, OpenEXR, KTX2, JPEG XR, DDS/WIC, MDL, and Substance adapters.

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

MaterialX is the authoritative material asset representation. Renderer adapters own GPU textures, descriptor bindings, shader compilation, and resource lifetime.
