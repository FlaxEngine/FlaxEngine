// Copyright (c) Wojciech Figat. All rights reserved.

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
    /// Describes work result value
    /// </summary>
    DECLARE_ENUM_5(Result, Ok, AssetLoadError, MissingReferences, LoadDataError, TaskFailed);

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
