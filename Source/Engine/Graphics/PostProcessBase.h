// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Enums.h"

class RenderTask;
class GPUTexture;
class GPUTextureView;
struct RenderContext;

/// <summary>
/// Post process effects base class.
/// </summary>
class PostProcessBase
{
protected:
    bool _isEnabled;
    bool _useSingleTarget;
    PostProcessEffectLocation _location;

    PostProcessBase()
        : _isEnabled(true)
        , _useSingleTarget(false)
        , _location(PostProcessEffectLocation::Default)
    {
    }

public:
    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~PostProcessBase()
    {
    }

public:
    /// <summary>
    /// Returns true if effect is enabled
    /// </summary>
    /// <returns>True if is enabled, otherwise false</returns>
    bool IsEnabled() const
    {
        return _isEnabled;
    }

    /// <summary>
    /// Set enabled state
    /// </summary>
    /// <param name="enabled">True if enable, otherwise disable effect</param>
    virtual void SetEnabled(bool enabled)
    {
        // Check if value will change
        if (_isEnabled != enabled)
        {
            // Change value
            _isEnabled = enabled;

            // Fire events
            if (_isEnabled)
                onEnable();
            else
                onDisable();
            onEnabledChanged();
        }
    }

    /// <summary>
    /// Returns true if effect is loaded and can be rendered
    /// </summary>
    /// <returns>True if is loaded</returns>
    virtual bool IsLoaded() const = 0;

    /// <summary>
    /// Returns true if effect is ready for rendering
    /// </summary>
    /// <returns>True if is ready</returns>
    bool IsReady() const
    {
        return _isEnabled && IsLoaded();
    }

    /// <summary>
    /// Gets a value indicating whether use a single render target as both input and output. Use this if your effect doesn't need to copy the input buffer to the output but can render directly to the single texture. Can be used to optimize game performance.
    /// </summary>
    bool GetUseSingleTarget() const
    {
        return _useSingleTarget;
    }

    /// <summary>
    /// Gets the effect rendering location within rendering pipeline.
    /// </summary>
    /// <returns>Render location.</returns>
    PostProcessEffectLocation GetLocation() const
    {
        return _location;
    }

public:
    /// <summary>
    /// Perform rendering
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="input">The input texture.</param>
    /// <param name="output">The output texture.</param>
    virtual void Render(RenderContext& renderContext, GPUTexture* input, GPUTexture* output) = 0;

protected:
    virtual void onEnable()
    {
    }

    virtual void onDisable()
    {
    }

    virtual void onEnabledChanged()
    {
    }
};
