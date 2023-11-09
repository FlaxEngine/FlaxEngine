#pragma once
#include "../Primitive/UIElement.h"
#include <Engine/Render2D/Render2D.h>

namespace Experimental
{
    /// <summary>
    /// Background Blur panel
    /// </summary>
    class BackgroundBlur : public UIElement
    {

    public:
        /// <summary>
        /// Gets or sets the blur strength. Defines how blurry the background is. Larger numbers increase blur, resulting in a larger runtime cost on the GPU.
        /// </summary>
        //API_FIELD( [EditorOrder(0), Limit(0, 100, 0.1f)])API_CLASS(Sealed, Attributes = "EditorOrder(0),Limit(0, 100, 0.1f)")
        float BlurStrength;

        /// <summary>
        /// If checked, the blur strength will be scaled with the control size, which makes it resolution-independent.
        /// </summary>
        //[EditorOrder(10)]
        bool BlurScaleWithSize = false;

        /// <inheritdoc />
        void Draw() override
        {
            UIElement::Draw();
            auto size = Transform.Size;
            auto strength = BlurStrength;
            if (BlurScaleWithSize)
                strength *= size.MinValue() / 1000.0f;
            if (strength > 0.00001f)
            {
                Render2D::DrawBlur(Rectangle(Float2::Zero, size), strength);
            }
        };
    };
}
