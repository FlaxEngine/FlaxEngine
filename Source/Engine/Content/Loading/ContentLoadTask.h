// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Threading/Task.h"

class Asset;
class LoadingThread;

/// <summary>
/// Describes content loading task object.
/// </summary>
class ContentLoadTask : public Task
{
    friend LoadingThread;

public:
    /// <summary>
    /// Describes work type
    /// </summary>
    DECLARE_ENUM_3(Type, Custom, LoadAsset, LoadAssetData);

    /// <summary>
    /// Describes work result value
    /// </summary>
    DECLARE_ENUM_5(Result, Ok, AssetLoadError, MissingReferences, LoadDataError, TaskFailed);

private:
    /// <summary>
    /// Task type
    /// </summary>
    Type _type;

protected:
    /// <summary>
    /// Initializes a new instance of the <see cref="ContentLoadTask"/> class.
    /// </summary>
    /// <param name="type">The task type.</param>
    ContentLoadTask(const Type type)
        : _type(type)
    {
    }

public:
    /// <summary>
    /// Gets a task type.
    /// </summary>
    FORCE_INLINE Type GetType() const
    {
        return _type;
    }

public:
    /// <summary>
    /// Checks if async task is loading given asset resource
    /// </summary>
    /// <param name="asset">Target asset to check</param>
    /// <returns>True if is loading that asset, otherwise false</returns>
    bool IsLoading(Asset* asset) const
    {
        return _type == Type::LoadAsset && HasReference((Object*)asset);
    }

protected:
    virtual Result run() = 0;

public:
    // [Task]
    String ToString() const override;

protected:
    // [Task]
    void Enqueue() override;
    bool Run() override;
};
