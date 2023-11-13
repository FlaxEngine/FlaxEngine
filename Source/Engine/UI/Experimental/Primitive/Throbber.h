#pragma once
#include "Engine/UI/Experimental/Common.h"

/// <summary>
/// Shows serverals zooming images in the row
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class FLAXENGINE_API Throbber : public UIElement
{
    DECLARE_SCRIPTING_TYPE(Throbber);
private:
    Array<Float3> States = Array<Float3>(3);
    int CurentElement = 0;
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
    API_FIELD() ImageBrush* Brush = nullptr;

    /// <summary>
    /// [ToDo] add summary
    /// </summary>
    API_FIELD() byte NumberOfPieces = 3;

    /// <summary>
    /// [ToDo] add summary
    /// </summary>
    API_FIELD() AnimateFlags Animate = (AnimateFlags)(AnimateFlags::Vertical | AnimateFlags::Horizontal | AnimateFlags::Alpha);

    /// <inheritdoc />
    API_FUNCTION() void OnPreCunstruct(bool isInDesigner) override
    {
        if (!Brush) 
        {
            Brush = New<ImageBrush>();
        }
        Brush->OnPreCunstruct(isInDesigner);
    };

    /// <inheritdoc />
    API_FUNCTION() virtual void OnCunstruct() override;
    /// <inheritdoc />
    API_FUNCTION() virtual void OnDraw() override;
    /// <inheritdoc />
    API_FUNCTION() virtual void OnDestruct() override;
};
