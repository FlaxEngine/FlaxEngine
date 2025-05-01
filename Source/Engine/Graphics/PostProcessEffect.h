// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/Script.h"
#include "Enums.h"

class GPUTexture;
class GPUContext;
struct RenderContext;

/// <summary>
/// Custom PostFx which can modify final image by processing it with material based filters. The base class for all post process effects used by the graphics pipeline. Allows to extend frame rendering logic and apply custom effects such as outline, night vision, contrast etc.
/// </summary>
/// <remarks>
/// Override this class and implement custom post fx logic. Use <b>MainRenderTask.Instance.AddCustomPostFx(myPostFx)</b> to attach your script to rendering or add script to camera actor.
/// </remarks>
API_CLASS(Abstract) class FLAXENGINE_API PostProcessEffect : public Script
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(PostProcessEffect);

public:
    /// <summary>
    /// Effect rendering location within rendering pipeline.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Post Process Effect\"), ExpandGroups") PostProcessEffectLocation Location = PostProcessEffectLocation::Default;

    /// <summary>
    /// True whether use a single render target as both input and output. Use this if your effect doesn't need to copy the input buffer to the output but can render directly to the single texture. Can be used to optimize game performance.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Post Process Effect\")") bool UseSingleTarget = false;

    /// <summary>
    /// Effect rendering order. Post effects are sorted before rendering (from the lowest order to the highest order).
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Post Process Effect\")") int32 Order = 0;

public:
    /// <summary>
    /// Gets a value indicating whether this effect can be rendered.
    /// </summary>
    API_FUNCTION() virtual bool CanRender() const
    {
        return GetEnabled();
    }
    
    /// <summary>
    /// Gets a value indicating whether this effect can be rendered.
    /// </summary>
    /// <param name="renderContext">The target render context.</param>
    API_FUNCTION() virtual bool CanRender(const RenderContext& renderContext) const
    {
        return CanRender();
    }

    /// <summary>
    /// Pre-rendering event called before scene rendering begin. Can be used to perform custom rendering or customize render view/setup.
    /// </summary>
    virtual void PreRender(GPUContext* context, RenderContext& renderContext)
    {
    }

    /// <summary>
    /// Performs custom postFx rendering.
    /// </summary>
    /// <param name="context">The GPU commands context.</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="input">The input texture.</param>
    /// <param name="output">The output texture.</param>
    API_FUNCTION() virtual void Render(GPUContext* context, API_PARAM(Ref) RenderContext& renderContext, GPUTexture* input, GPUTexture* output)
    {
    }
};
