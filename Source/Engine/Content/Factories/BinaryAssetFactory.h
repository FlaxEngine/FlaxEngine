// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "IAssetFactory.h"
#include "Engine/Content/Upgraders/BinaryAssetUpgrader.h"
#include "Engine/Content/AssetInfo.h"
#include "Engine/Scripting/ScriptingObject.h"

class BinaryAsset;
class FlaxStorage;

/// <summary>
/// The binary assets factory base class.
/// </summary>
/// <seealso cref="IAssetFactory" />
class BinaryAssetFactoryBase : public IAssetFactory
{
public:

    /// <summary>
    /// Initializes the specified asset. It's called in background before actual asset loading.
    /// </summary>
    /// <param name="asset">The asset.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Init(BinaryAsset* asset);

protected:

    virtual BinaryAsset* Create(const AssetInfo& info) = 0;
    virtual bool IsVersionSupported(uint32 serializedVersion) const = 0;
#if COMPILE_WITH_ASSET_UPGRADERS
    bool UpgradeAsset(const AssetInfo& info, FlaxStorage* storage, AssetMigrationContext& context);
#endif

public:

    // [IAssetFactory]
    Asset* New(const AssetInfo& info) override;
    Asset* NewVirtual(const AssetInfo& info) override;
};

/// <summary>
/// The binary assets factory.
/// </summary>
/// <seealso cref="BinaryAssetFactoryBase" />
template<typename T>
class BinaryAssetFactory : public BinaryAssetFactoryBase
{
public:

    // [BinaryAssetFactoryBase]
    bool IsVersionSupported(uint32 serializedVersion) const override
    {
        return T::SerializedVersion == serializedVersion;
    }

protected:

    // [BinaryAssetFactoryBase]
    BinaryAsset* Create(const AssetInfo& info) override
    {
        ScriptingObjectSpawnParams params(info.ID, T::TypeInitializer);
        return ::New<T>(params, &info);
    }
};

#define REGISTER_BINARY_ASSET(type, typeName, upgrader, supportsVirtualAssets) \
	const String type::TypeName = TEXT(typeName); \
	class CONCAT_MACROS(Factory, type) : public BinaryAssetFactory<type> \
	{ \
		private: \
		IAssetUpgrader* _upgrader = upgrader; \
		public: \
		CONCAT_MACROS(Factory, type)() { IAssetFactory::Get().Add(type::TypeName, this); } \
		~CONCAT_MACROS(Factory, type)() { if (_upgrader) Delete(_upgrader); IAssetFactory::Get().Remove(type::TypeName); } \
		bool SupportsVirtualAssets() const override { return supportsVirtualAssets; } \
		IAssetUpgrader* GetUpgrader() const override { return _upgrader; } \
	}; \
	static CONCAT_MACROS(Factory, type) CONCAT_MACROS(CFactory, type)
