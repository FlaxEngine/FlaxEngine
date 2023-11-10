#pragma once
#include "UIElement.h"

/// <summary>
/// Shows serverals zooming images in the row
/// </summary>
API_CLASS()
class FLAXENGINE_API Throbber : public UIElement, public ISlot
{
    DECLARE_SCRIPTING_TYPE(Throbber);

    /// <inheritdoc />
    void OnDraw() override;
};
