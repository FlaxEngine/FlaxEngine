// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "IAssetFactory.h"
#include "../JsonAsset.h"
#include "Engine/Content/AssetInfo.h"

/// <summary>
/// The Json assets factory base class.
/// </summary>
/// <seealso cref="IAssetFactory" />
class FLAXENGINE_API JsonAssetFactoryBase : public IAssetFactory
{
protected:
    virtual JsonAssetBase* Create(const AssetInfo& info) = 0;

public:
    // [IAssetFactory]
    Asset* New(const AssetInfo& info) override
    {
        return Create(info);
    }

    Asset* NewVirtual(const AssetInfo& info) override
    {
        return Create(info);
    }
};

/// <summary>
/// The Json assets factory.
/// </summary>
/// <seealso cref="JsonAssetFactoryBase" />
template<typename T>
class JsonAssetFactory : public JsonAssetFactoryBase
{
protected:
    // [JsonAssetFactoryBase]
    JsonAssetBase* Create(const AssetInfo& info) override
    {
        ScriptingObjectSpawnParams params(info.ID, T::TypeInitializer);
        return ::New<T>(params, &info);
    }
};

#define REGISTER_JSON_ASSET(type, typeName, supportsVirtualAssets) \
	const String type::TypeName = TEXT(typeName); \
	class CONCAT_MACROS(Factory, type) : public JsonAssetFactory<type> \
	{ \
		public: \
		CONCAT_MACROS(Factory, type)() { IAssetFactory::Get().Add(type::TypeName, this); } \
		~CONCAT_MACROS(Factory, type)() { IAssetFactory::Get().Remove(type::TypeName); } \
		bool SupportsVirtualAssets() const override { return supportsVirtualAssets; } \
	}; \
	static CONCAT_MACROS(Factory, type) CONCAT_MACROS(CFactory, type)
