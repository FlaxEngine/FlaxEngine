// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialBase.h"

/// <summary>
/// Instance of the <seealso cref="Material" /> with custom set of material parameter values.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API MaterialInstance : public MaterialBase
{
    DECLARE_BINARY_ASSET_HEADER(MaterialInstance, 4);
private:
    MaterialBase* _baseMaterial = nullptr;

public:
    /// <summary>
    /// Gets the base material. If value gets changed parameters collection is restored to the default values of the new material.
    /// </summary>
    API_PROPERTY() FORCE_INLINE MaterialBase* GetBaseMaterial() const
    {
        return _baseMaterial;
    }

    /// <summary>
    /// Sets the base material. If value gets changed parameters collection is restored to the default values of the new material.
    /// </summary>
    /// <param name="baseMaterial">The base material.</param>
    API_PROPERTY() void SetBaseMaterial(MaterialBase* baseMaterial);

    /// <summary>
    /// Resets all parameters back to the base material (including disabling parameter overrides).
    /// </summary>
    API_FUNCTION() void ResetParameters();

private:
    void OnBaseSet();
    void OnBaseUnset();
    void OnBaseUnloaded(Asset* p);
    void OnBaseParamsChanged();

public:
    // [MaterialBase]
    bool IsMaterialInstance() const override;
#if USE_EDITOR
    void GetReferences(Array<Guid>& assets, Array<String>& files) const override;
    bool Save(const StringView& path = StringView::Empty) override;
#endif

    // [IMaterial]
    const MaterialInfo& GetInfo() const override;
    GPUShader* GetShader() const override;
    bool IsReady() const override;
    DrawPass GetDrawModes() const override;
    bool CanUseLightmap() const override;
    bool CanUseInstancing(InstancingHandler& handler) const override;
    void Bind(BindParameters& params) override;

protected:
    // [MaterialBase]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
