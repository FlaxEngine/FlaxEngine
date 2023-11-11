#pragma once
#include "Engine/UI/Experimental/Types/UIElement.h"
#include "Engine/UI/Experimental/Types/IBrush.h"

/// <summary>
/// Shows serverals zooming images in the row
/// </summary>
API_CLASS()
class FLAXENGINE_API Throbber : public UIElement
{
    DECLARE_SCRIPTING_TYPE(Throbber);
private:
    Array<Float3> States;
    int CurentElement;
public:
    API_ENUM()
    enum AnimateFlags
    {
        None = 0,
        Vertical = 1,
        Horizontal = 2,
        Alpha = 4,
    };
    /// <summary>
    /// The brush
    /// </summary>
    API_FIELD() IBrush* Brush;

    /// <summary>
    /// [ToDo] add summary
    /// </summary>
    API_FIELD() byte NumberOfPieces;

    /// <summary>
    /// [ToDo] add summary
    /// </summary>
    API_FIELD() byte NumberOfPieces;

    /// <summary>
    /// [ToDo] add summary
    /// </summary>
    API_FIELD() AnimateFlags Animate = (AnimateFlags)(AnimateFlags::Vertical | AnimateFlags::Horizontal | AnimateFlags::Alpha);

    /// <inheritdoc />
    void OnDraw() override;

    API_FUNCTION() virtual Float2 GetDesiredSize() override 
    {
        return Float2(NumberOfPieces * GetSlot()->GetSize().X, GetSlot()->GetSize().Y);
    }
};
