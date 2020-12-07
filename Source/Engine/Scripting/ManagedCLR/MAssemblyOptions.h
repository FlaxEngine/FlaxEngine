// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// The options for creation of the managed assembly.
/// </summary>
struct MAssemblyOptions
{
public:

    /// <summary>
    /// Should assembly cache classes on Load method.
    /// </summary>
    int32 PreCacheOnLoad : 1;

    /// <summary>
    /// Locks DLL/exe file with managed code.
    /// </summary>
    int32 KeepManagedFileLocked : 1;

    /// <summary>
    /// Initial dictionary size (prevents memory partition for bigger assemblies).
    /// </summary>
    int32 DictionaryInitialSize;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="MAssemblyOptions"/> struct.
    /// </summary>
    MAssemblyOptions()
        : PreCacheOnLoad(true)
        , KeepManagedFileLocked(false)
        , DictionaryInitialSize(1024)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="MAssemblyOptions"/> struct.
    /// </summary>
    /// <param name="preCacheOnLoad">if set to <c>true</c> to precache assembly metadata on load.</param>
    /// <param name="keepManagedFileLocked">if set to <c>true</c> keep managed file locked after load.</param>
    /// <param name="dictionaryInitialSize">Initial size of the dictionary for the classes and metadata.</param>
    MAssemblyOptions(bool preCacheOnLoad, bool keepManagedFileLocked = false, int32 dictionaryInitialSize = 1024)
        : PreCacheOnLoad(preCacheOnLoad)
        , KeepManagedFileLocked(keepManagedFileLocked)
        , DictionaryInitialSize(dictionaryInitialSize)
    {
    }
};
