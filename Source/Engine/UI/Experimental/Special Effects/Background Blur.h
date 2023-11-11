#pragma once
#include "Engine/UI/Experimental/Types/UIElement.h"

/// <summary>
/// Background Blur panel
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class FLAXENGINE_API BackgroundBlur : public UIElement
{
    DECLARE_SCRIPTING_TYPE(BackgroundBlur);
public:

    /// <summary>
    /// Gets or sets the blur strength. Defines how blurry the background is. Larger numbers increase blur, resulting in a larger runtime cost on the GPU.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(0), Limit(0, 100, 0.1f)")
        float BlurStrength = 1.0f;

    /// <summary>
    /// If checked, the blur strength will be scaled with the control size, which makes it resolution-independent.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(1)")
        bool BlurScaleWithSize = false;

    /// <inheritdoc />
    void OnDraw() override;
};
