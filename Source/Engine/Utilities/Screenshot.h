// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/StringView.h"
#include "Engine/Scripting/ScriptingType.h"

class RenderTask;
class SceneRenderTask;
class GPUTexture;

/// <summary>
/// The utility class for capturing game screenshots.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Screenshot
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Screenshot);

    /// <summary>
    /// Captures the specified render target contents and saves it to the file.
    /// Remember that downloading data from the GPU may take a while so screenshot may be taken one or more frames later due to latency.
    /// Staging textures are saved immediately.
    /// </summary>
    /// <param name="target">The target render target to capture it's contents.</param>
    /// <param name="path">The custom file location. Use null or empty to use default one.</param>
    API_FUNCTION() static void Capture(GPUTexture* target, const StringView& path = StringView::Empty);

    /// <summary>
    /// Captures the specified render task backbuffer contents and saves it to the file.
    /// Remember that downloading data from the GPU may take a while so screenshot may be taken one or more frames later due to latency.
    /// </summary>
    /// <param name="target">The target task to capture it's backbuffer.</param>
    /// <param name="path">The custom file location. Use null or empty to use default one.</param>
    API_FUNCTION() static void Capture(SceneRenderTask* target = nullptr, const StringView& path = StringView::Empty);

    /// <summary>
    /// Captures the main render task backbuffer contents and saves it to the file.
    /// Remember that downloading data from the GPU may take a while so screenshot may be taken one or more frames later due to latency.
    /// </summary>
    /// <param name="path">The custom file location. Use null or empty to use default one.</param>
    API_FUNCTION(Attributes="DebugCommand") static void Capture(const StringView& path = StringView::Empty);
};
