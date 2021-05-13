// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Threading/Task.h"
#include "Engine/Core/Types/String.h"

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
    DECLARE_ENUM_4(Result, Ok, AssetLoadError, MissingReferences, LoadDataError);

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
    /// <returns>The type.</returns>
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
    String ToString() const override
    {
        return String::Format(TEXT("Content Load Task {0} ({1})"),
                              ToString(GetType()),
                              ::ToString(GetState())
        );
    }

protected:

    // [Task]
    void Enqueue() override;
    bool Run() override;
};
