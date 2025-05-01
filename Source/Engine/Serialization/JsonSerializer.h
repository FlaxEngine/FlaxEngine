// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/ISerializable.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Objects serialization tool (json format).
/// </summary>
API_CLASS(Static, Namespace="FlaxEngine.Json") class FLAXENGINE_API JsonSerializer
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(JsonSerializer);

    /// <summary>
    /// Performs object Json serialization to the raw bytes.
    /// </summary>
    /// <param name="obj">The object to serialize (can be null).</param>
    /// <returns>The output data.</returns>
    API_FUNCTION() static Array<byte> SaveToBytes(ISerializable* obj);

    /// <summary>
    /// Performs object Json deserialization from the raw bytes.
    /// </summary>
    /// <param name="obj">The object to deserialize (can be null).</param>
    /// <param name="data">The source data to read from.</param>
    /// <param name="engineBuild">The engine build number of the saved data. Used to resolve old object formats when loading deprecated data.</param>
    FORCE_INLINE static void LoadFromBytes(ISerializable* obj, const Array<byte>& data, int32 engineBuild)
    {
        LoadFromBytes(obj, Span<byte>(data.Get(), data.Count()), engineBuild);
    }

    /// <summary>
    /// Performs object Json deserialization from the raw bytes.
    /// </summary>
    /// <param name="obj">The object to deserialize (can be null).</param>
    /// <param name="data">The source data to read from.</param>
    /// <param name="engineBuild">The engine build number of the saved data. Used to resolve old object formats when loading deprecated data.</param>
    API_FUNCTION() static void LoadFromBytes(ISerializable* obj, const Span<byte>& data, int32 engineBuild);
};
