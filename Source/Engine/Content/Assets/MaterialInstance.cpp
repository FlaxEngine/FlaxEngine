// Copyright (c) Wojciech Figat. All rights reserved.

#include "MaterialInstance.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Upgraders/MaterialInstanceUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Threading/Threading.h"
#if USE_EDITOR
#include "Engine/Serialization/MemoryWriteStream.h"
#endif

REGISTER_BINARY_ASSET_WITH_UPGRADER(MaterialInstance, "FlaxEngine.MaterialInstance", MaterialInstanceUpgrader, true);

MaterialInstance::MaterialInstance(const SpawnParams& params, const AssetInfo* info)
    : MaterialBase(params, info)
{
}

void MaterialInstance::OnBaseSet()
{
    ScopeLock lock(_baseMaterial->Locker);
    ASSERT(_baseMaterial->IsLoaded());

    _baseMaterial->AddReference();
    _baseMaterial->OnUnloaded.Bind<MaterialInstance, &MaterialInstance::OnBaseUnloaded>(this);
    _baseMaterial->ParamsChanged.Bind<MaterialInstance, &MaterialInstance::OnBaseParamsChanged>(this);

    // Sync parameters with the base parameters to ensure all data is valid for rendering (constants offset and resource register)
    MaterialParams& baseParams = _baseMaterial->Params;
    Params._versionHash = 0;
    if (Params.Count() != baseParams.Count())
    {
        // Params changed
        OnBaseParamsChanged();
        return;
    }
    for (int32 i = 0; i < Params.Count(); i++)
    {
        MaterialParameter& param = Params[i];
        MaterialParameter& baseParam = baseParams[i];

        if (param.GetID() != baseParam.GetID() || param.GetParameterType() != baseParam.GetParameterType())
        {
            // Params changed
            OnBaseParamsChanged();
            return;
        }

        param._isPublic = baseParam._isPublic;
        param._registerIndex = baseParam._registerIndex;
        param._offset = baseParam._offset;
        param._name = baseParam._name;
    }

    // Params are valid
    Params._versionHash = baseParams._versionHash;
    ParamsChanged();
}

void MaterialInstance::OnBaseUnset()
{
    _baseMaterial->RemoveReference();
    _baseMaterial->OnUnloaded.Unbind<MaterialInstance, &MaterialInstance::OnBaseUnloaded>(this);
    _baseMaterial->ParamsChanged.Unbind<MaterialInstance, &MaterialInstance::OnBaseParamsChanged>(this);
}

void MaterialInstance::OnBaseUnloaded(Asset* p)
{
    SetBaseMaterial(nullptr);
}

void MaterialInstance::OnBaseParamsChanged()
{
    ScopeLock lock(Locker);

    // Skip if version has not been changed and the hash is the same
    auto baseParams = &_baseMaterial->Params;
    if (Params.GetVersionHash() == baseParams->GetVersionHash())
        return;

    //LOG(Info, "Updating material instance params \'{0}\' (base: \'{1}\')", ToString(), _baseMaterial->ToString());

    // Cache previous parameters
    MaterialParams oldParams;
    Params.Clone(oldParams);

    // Get the newest parameters
    baseParams->Clone(Params);

    // Override all public parameters by default
    for (auto& param : Params)
        param.SetIsOverride(param.IsPublic());

    // Copy previous parameters values
    for (int32 i = 0; i < oldParams.Count(); i++)
    {
        const MaterialParameter& oldParam = oldParams[i];
        MaterialParameter* param = Params.Get(oldParam.GetParameterID());
        if (param)
        {
            // Check type
            if (oldParam.GetParameterType() == param->GetParameterType())
            {
                // Restore value
                const Variant value = oldParam.GetValue();
                param->SetValue(value);
                param->SetIsOverride(oldParam.IsOverride());
            }
            else
            {
                LOG(Info, "Param {0} changed type from {1}", param->ToString(), oldParam.ToString());
            }
        }
    }

    ParamsChanged();
}

bool MaterialInstance::IsMaterialInstance() const
{
    return true;
}

#if USE_EDITOR

void MaterialInstance::GetReferences(Array<Guid>& assets, Array<String>& files) const
{
    MaterialBase::GetReferences(assets, files);
    if (_baseMaterial)
        assets.Add(_baseMaterial->GetID());
}

#endif

const MaterialInfo& MaterialInstance::GetInfo() const
{
    if (_baseMaterial)
        return _baseMaterial->GetInfo();

    static MaterialInfo EmptyInfo;
    return EmptyInfo;
}

GPUShader* MaterialInstance::GetShader() const
{
    return _baseMaterial ? _baseMaterial->GetShader() : nullptr;
}

bool MaterialInstance::IsReady() const
{
    return IsLoaded() && _baseMaterial && _baseMaterial->IsReady();
}

DrawPass MaterialInstance::GetDrawModes() const
{
    if (_baseMaterial)
        return _baseMaterial->GetDrawModes();
    return DrawPass::None;
}

bool MaterialInstance::CanUseLightmap() const
{
    return _baseMaterial && _baseMaterial->CanUseLightmap();
}

bool MaterialInstance::CanUseInstancing(InstancingHandler& handler) const
{
    return _baseMaterial && _baseMaterial->CanUseInstancing(handler);
}

void MaterialInstance::Bind(BindParameters& params)
{
    //ASSERT(IsReady());
    //ASSERT(_baseMaterial->Params.GetVersionHash() == Params.GetVersionHash());

    auto lastLink = params.ParamsLink;
    MaterialParamsLink link;
    link.This = &Params;
    if (lastLink)
    {
        while (lastLink->Down)
            lastLink = lastLink->Down;
        lastLink->Down = &link;
    }
    else
    {
        params.ParamsLink = &link;
    }
    link.Up = lastLink;
    link.Down = nullptr;

    _baseMaterial->Bind(params);

    if (lastLink)
    {
        lastLink->Down = nullptr;
    }
    else
    {
        params.ParamsLink = nullptr;
    }
}

Asset::LoadResult MaterialInstance::load()
{
    // Get main chunk
    auto chunk0 = GetChunk(0);
    if (chunk0 == nullptr || chunk0->IsMissing())
        return LoadResult::MissingDataChunk;
    MemoryReadStream headerStream(chunk0->Get(), chunk0->Size());

    // Load base material
    Guid baseMaterialId;
    headerStream.Read(baseMaterialId);
    auto baseMaterial = Content::LoadAsync<MaterialBase>(baseMaterialId);

    // Load parameters
    if (Params.Load(&headerStream))
    {
        LOG(Warning, "Cannot load material parameters.");
        return LoadResult::CannotLoadData;
    }

    if (baseMaterial && !baseMaterial->WaitForLoaded())
    {
        _baseMaterial = baseMaterial;
        OnBaseSet();
    }
    else
    {
        // Clear parameters if has no material loaded
        _baseMaterial = nullptr;
        Params.Dispose();
        ParamsChanged();
    }

    return LoadResult::Ok;
}

void MaterialInstance::unload(bool isReloading)
{
    if (_baseMaterial)
    {
        OnBaseUnset();
        _baseMaterial = nullptr;
    }
    Params.Dispose();
}

AssetChunksFlag MaterialInstance::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}

void MaterialInstance::SetBaseMaterial(MaterialBase* baseMaterial)
{
    if (baseMaterial != nullptr && baseMaterial->WaitForLoaded())
    {
        LOG(Warning, "Cannot set base material of {0} to {1} because it failed to load.", ToString(), baseMaterial->ToString());
        return;
    }

    ScopeLock lock(Locker);

    if (baseMaterial == _baseMaterial)
        return;

#if !BUILD_RELEASE
    // Prevent recursion
    auto mi = Cast<MaterialInstance>(baseMaterial);
    while (mi)
    {
        if (mi == this)
        {
            LOG(Error, "Cannot set base material of {0} to {1} because it's recursive.", ToString(), baseMaterial->ToString());
            return;
        }
        mi = Cast<MaterialInstance>(mi->GetBaseMaterial());
    }
#endif

    // Release previous parameters
    Params.Dispose();

    // Set new value
    if (_baseMaterial)
    {
        OnBaseUnset();
    }
    _baseMaterial = baseMaterial;
    if (baseMaterial)
    {
        _baseMaterial = baseMaterial;
        OnBaseSet();
    }
}

void MaterialInstance::ResetParameters()
{
    for (auto& param : Params)
    {
        param.SetIsOverride(false);
    }
}

#if USE_EDITOR

bool MaterialInstance::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
    ScopeLock lock(Locker);

    // Save instance data
    MemoryWriteStream stream(512);
    {
        // Save base material ID
        const Guid baseMaterialId = _baseMaterial ? _baseMaterial->GetID() : Guid::Empty;
        stream.Write(baseMaterialId);

        // Save parameters
        Params.Save(&stream);
    }
    SetChunk(0, ToSpan(stream));

    // Setup asset data
    AssetInitData data;
    data.SerializedVersion = 4;

    // Save data
    const bool saveResult = path.HasChars() ? SaveAsset(path, data) : SaveAsset(data, true);
    if (saveResult)
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

#endif
