// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "MaterialBase.h"
#include "MaterialInstance.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"

REGISTER_BINARY_ASSET_ABSTRACT(MaterialBase, "FlaxEngine.MaterialBase");

MaterialBase::MaterialBase(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

Variant MaterialBase::GetParameterValue(const StringView& name)
{
    const auto param = Params.Get(name);
    if (param)
    {
        return param->GetValue();
    }
    LOG(Warning, "Missing material parameter '{0}' in material {1}", String(name), ToString());
    return Variant::Null;
}

void MaterialBase::SetParameterValue(const StringView& name, const Variant& value, bool warnIfMissing, bool warnIfWrongType)
{
    const auto param = Params.Get(name);
    if (param)
    {
        if (Variant::CanCast(value, param->GetValue().Type))
        {
            param->SetValue(value);
            param->SetIsOverride(true);
        }
        else if (warnIfWrongType)
        {
            LOG(Warning, "Material parameter '{0}' in material {1} is type '{2}' and not type '{3}'.", String(name), ToString(), param->GetValue().Type, value.Type);
        }
    }
    else if (warnIfMissing)
    {
        LOG(Warning, "Missing material parameter '{0}' in material {1}", String(name), ToString());
    }
}

MaterialInstance* MaterialBase::CreateVirtualInstance()
{
    auto instance = Content::CreateVirtualAsset<MaterialInstance>();
    instance->SetBaseMaterial(this);
    return instance;
}

#if USE_EDITOR

void MaterialBase::GetReferences(Array<Guid>& assets, Array<String>& files) const
{
    BinaryAsset::GetReferences(assets, files);
    Params.GetReferences(assets);
}

#endif
