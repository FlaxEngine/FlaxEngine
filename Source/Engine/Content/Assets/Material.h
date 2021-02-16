// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialBase.h"
#include "Engine/Graphics/Shaders/Cache/ShaderAssetBase.h"

class MaterialShader;

/// <summary>
/// Material asset that contains shader for rendering models on the GPU.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API Material : public ShaderAssetTypeBase<MaterialBase>
{
DECLARE_BINARY_ASSET_HEADER(Material, ShadersSerializedVersion);
private:

    MaterialShader* _materialShader = nullptr;

public:

    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <param name="createDefaultIfMissing">True if create default surface if missing.</param>
    /// <returns>The output surface data, or empty if failed to load.</returns>
    API_FUNCTION() BytesContainer LoadSurface(bool createDefaultIfMissing);

#if USE_EDITOR

    /// <summary>
    /// Updates the material surface (save new one, discard cached data, reload asset).
    /// </summary>
    /// <param name="data">The surface graph data.</param>
    /// <param name="info">The material info structure.</param>
    /// <returns>True if cannot save it, otherwise false.</returns>
    API_FUNCTION() bool SaveSurface(BytesContainer& data, const MaterialInfo& info);

#endif

public:

    // [MaterialBase]
    bool IsMaterialInstance() const override;

    // [IMaterial]
    const MaterialInfo& GetInfo() const override;
    bool IsReady() const override;
    DrawPass GetDrawModes() const override;
    bool CanUseLightmap() const override;
    bool CanUseInstancing(InstancingHandler& handler) const override;
    void Bind(BindParameters& params) override;

    // [ShaderAssetBase]
#if USE_EDITOR
    void InitCompilationOptions(ShaderCompilationOptions& options) override;
#endif

protected:

    // [MaterialBase]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
#if USE_EDITOR
    void OnDependencyModified(BinaryAsset* asset) override;
#endif
};
