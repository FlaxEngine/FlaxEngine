// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// The options for creation of the managed assembly.
/// </summary>
struct MAssemblyOptions
{
    /// <summary>
    /// Should assembly cache classes on Load method.
    /// </summary>
    int32 PreCacheOnLoad : 1;

    /// <summary>
    /// Locks DLL/exe file with managed code.
    /// </summary>
    int32 KeepManagedFileLocked : 1;

    /// <summary>
    /// Initializes a new instance of the <see cref="MAssemblyOptions"/> struct.
    /// </summary>
    /// <param name="preCacheOnLoad">if set to <c>true</c> to precache assembly metadata on load.</param>
    /// <param name="keepManagedFileLocked">if set to <c>true</c> keep managed file locked after load.</param>
    MAssemblyOptions(bool preCacheOnLoad = true, bool keepManagedFileLocked = false)
        : PreCacheOnLoad(preCacheOnLoad)
        , KeepManagedFileLocked(keepManagedFileLocked)
    {
    }
};
