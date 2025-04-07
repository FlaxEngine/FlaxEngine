// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/Dictionary.h"

struct AssetInfo;
class Asset;
class IAssetUpgrader;

/// <summary>
/// The asset objects factory.
/// </summary>
class FLAXENGINE_API IAssetFactory
{
public:
    typedef Dictionary<StringView, IAssetFactory*> Collection;

    /// <summary>
    /// Gets the all registered assets factories. Key is asset typename, value is the factory object.
    /// </summary>
    static Collection& Get();

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="IAssetFactory"/> class.
    /// </summary>
    virtual ~IAssetFactory()
    {
    }

public:
    /// <summary>
    /// Determines whenever the virtual assets are supported by this asset tpe factory.
    /// </summary>
    /// <returns>True if can create virtual assets, otherwise false.</returns>
    virtual bool SupportsVirtualAssets() const
    {
        return false;
    }

    /// <summary>
    /// Creates new asset instance.
    /// </summary>
    /// <param name="info">The asset info structure.</param>
    /// <returns>Created asset object.</returns>
    virtual Asset* New(const AssetInfo& info) = 0;

    /// <summary>
    /// Creates new virtual asset instance. Virtual assets are temporary and exist until application exit.
    /// </summary>
    /// <param name="info">The asset info structure.</param>
    /// <returns>Created asset object.</returns>
    virtual Asset* NewVirtual(const AssetInfo& info) = 0;

    /// <summary>
    /// Gets the asset upgrader.
    /// </summary>
    /// <returns>Asset upgrader, or null if not used.</returns>
    virtual IAssetUpgrader* GetUpgrader() const
    {
        return nullptr;
    }
};
