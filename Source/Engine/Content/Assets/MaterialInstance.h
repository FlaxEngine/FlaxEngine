// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
    /// <returns>The base material.</returns>
    API_PROPERTY() FORCE_INLINE MaterialBase* GetBaseMaterial() const
    {
        return _baseMaterial;
    }

    /// <summary>
    /// Sets the base material. If value gets changed parameters collection is restored to the default values of the new material.
    /// </summary>
    /// <param name="baseMaterial">The base material.</param>
    API_PROPERTY() void SetBaseMaterial(MaterialBase* baseMaterial);

#if USE_EDITOR

    /// <summary>
    /// Saves this asset to the file. Supported only in Editor.
    /// </summary>
    /// <remarks>If you use saving with the GPU mesh data then the call has to be provided from the thread other than the main game thread.</remarks>
    /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
    /// <returns>True if cannot save data, otherwise false.</returns>
    API_FUNCTION() bool Save(const StringView& path = StringView::Empty);

#endif

private:

    void OnBaseSet();
    void OnBaseUnset();
    void OnBaseUnloaded(Asset* p);
    void OnBaseParamsChanged();
    void OnUnload();

public:

    // [MaterialBase]
    bool IsMaterialInstance() const override;
#if USE_EDITOR
    void GetReferences(Array<Guid>& output) const override;
#endif

    // [IMaterial]
    const MaterialInfo& GetInfo() const override;
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
