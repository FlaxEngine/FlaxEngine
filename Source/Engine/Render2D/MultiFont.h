#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Font.h"

/// <summary>
/// The font segment info generated during text processing.
/// </summary>
API_STRUCT(NoDefault) struct MultiFontSegmentCache
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(MultiFontSegmentCache);

    /// <summary>
    /// The root position of the segment (upper left corner), relative to line.
    /// </summary>
    API_FIELD() Float2 Location;

    /// <summary>
    /// The height of the current segment
    /// </summary>
    API_FIELD() float Height;

    /// <summary>
    /// The first character index (from the input text).
    /// </summary>
    API_FIELD() int32 FirstCharIndex;

    /// <summary>
    /// The last character index (from the input text), inclusive.
    /// </summary>
    API_FIELD() int32 LastCharIndex;

    /// <summary>
    /// The index of the font to render with
    /// </summary>
    API_FIELD() int32 FontIndex;
};

template<>
struct TIsPODType<MultiFontSegmentCache>
{
    enum { Value = true };
};

/// <summary>
/// Line of font segments info generated during text processing.
/// </summary>
API_STRUCT(NoDefault) struct MultiFontLineCache
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(MultiFontLineCache);

    /// <summary>
    /// The root position of the line (upper left corner).
    /// </summary>
    API_FIELD() Float2 Location;

    /// <summary>
    /// The line bounds (width and height).
    /// </summary>
    API_FIELD() Float2 Size;

    /// <summary>
    /// The maximum ascendent of the line. 
    /// </summary>
    API_FIELD() float MaxAscender;

    /// <summary>
    /// The index of the font to render with
    /// </summary>
    API_FIELD() Array<MultiFontSegmentCache> Segments;
};

API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API MultiFont : public ManagedScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(MultiFont);
};
