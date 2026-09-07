# mesh-material-lib

Portable MaterialX-first material contracts for renderer and DCC integrations.

## Targets

- `mesh_material::core`: portable asset IDs, math/PBR types, material contracts, factory, and built-in glTF/USD Preview Surface translators.
- `mesh_material::materialx`: optional MaterialX XML validation and fixed-PBR runtime compiler.
- Future optional targets: OpenEXR, KTX2, JPEG XR, DDS/WIC, MDL, and Substance adapters.

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
