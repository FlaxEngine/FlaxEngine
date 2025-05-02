// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Interface for GUI data object container.
/// </summary>
class IGuiData
{
public:

    /// <summary>
    /// The GUI data types.
    /// </summary>
    enum class Type
    {
        Unknown,
        Text,
        Files
    };

public:

    /// <summary>
    /// Gets the data type.
    /// </summary>
    /// <returns>The data type.</returns>
    virtual Type GetType() const = 0;

    /// <summary>
    /// Gets data value as text.
    /// </summary>
    /// <returns>The text.</returns>
    virtual String GetAsText() const = 0;

    /// <summary>
    /// Gets data value as array of file paths.
    /// </summary>
    /// <param name="files">An array to fill with paths.</param>
    virtual void GetAsFiles(Array<String>* files) const = 0;
};
