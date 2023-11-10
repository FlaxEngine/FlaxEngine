#pragma once
#include "UIElement.h"

/// <summary>
/// Shows serverals zooming images in the circle
/// </summary>
API_CLASS()
class FLAXENGINE_API CircularThrobber : public UIElement , public ISlot
{
    DECLARE_SCRIPTING_TYPE(CircularThrobber);

    /// <inheritdoc />
    void Draw() override;
};
