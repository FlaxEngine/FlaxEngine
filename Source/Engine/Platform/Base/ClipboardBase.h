// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/Array.h"

API_INJECT_CODE(cpp, "#include \"Engine/Platform/Clipboard.h\"");

/// <summary>
/// Native platform clipboard service.
/// </summary>
API_CLASS(Static, Name="Clipboard", Tag="NativeInvokeUseName")
class FLAXENGINE_API ClipboardBase
{
public:
    static struct FLAXENGINE_API ScriptingTypeInitializer TypeInitializer;

    /// <summary>
    /// Clear the clipboard contents.
    /// </summary>
    API_FUNCTION() static void Clear()
    {
    }

    /// <summary>
    /// Sets text to the clipboard.
    /// </summary>
    /// <param name="text">The text to set.</param>
    API_PROPERTY() static void SetText(const StringView& text)
    {
    }

    /// <summary>
    /// Sets the raw bytes data to the clipboard.
    /// </summary>
    /// <param name="data">The data to set.</param>
    API_PROPERTY() static void SetRawData(const Span<byte>& data)
    {
    }

    /// <summary>
    /// Sets the files to the clipboard.
    /// </summary>
    /// <param name="files">The list of file paths.</param>
    API_PROPERTY() static void SetFiles(const Array<String>& files)
    {
    }

    /// <summary>
    /// Gets the text from the clipboard.
    /// </summary>
    /// <returns>The result text (or empty if clipboard doesn't have valid data).</returns>
    API_PROPERTY() static String GetText()
    {
        return String::Empty;
    }

    /// <summary>
    /// Gets the raw bytes data from the clipboard.
    /// </summary>
    /// <returns>The result data (or empty if clipboard doesn't have valid data).</returns>
    API_PROPERTY() static Array<byte> GetRawData()
    {
        return Array<byte>();
    }

    /// <summary>
    /// Gets the file paths from the clipboard.
    /// </summary>
    /// <returns>The output list of file paths (or empty if clipboard doesn't have valid data).</returns>
    API_PROPERTY() static Array<String> GetFiles()
    {
        return Array<String>();
    }
};
