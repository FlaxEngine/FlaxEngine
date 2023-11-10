#pragma once
#include "Engine/UI/Experimental/Types/UIElement.h"

/// <summary>
/// Shows serverals zooming images in the row
/// </summary>
API_CLASS()
class FLAXENGINE_API Throbber : public UIElement
{
    DECLARE_SCRIPTING_TYPE(Throbber);

    /// <inheritdoc />
    void OnDraw() override;
};
