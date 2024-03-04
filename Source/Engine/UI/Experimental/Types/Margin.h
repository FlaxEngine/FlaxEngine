#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Rectangle.h"

API_STRUCT(Namespace = "FlaxEngine.Experimental.UI")
struct FLAXENGINE_API UIMargin
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(UIMargin);
public:
    API_FIELD() float Left;
    API_FIELD() float Right;
    API_FIELD() float Top;
    API_FIELD() float Bottom;

    FORCE_INLINE bool operator==(const UIMargin& Other) const
    {
        return Left == Other.Left && Right == Other.Right && Top == Other.Top && Bottom == Other.Bottom;
    }

    FORCE_INLINE bool operator!=(const UIMargin& Other) const
    {
        return !(*this == Other);
    }
};
