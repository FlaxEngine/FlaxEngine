// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

/// <summary>
/// Window closing reasons.
/// </summary>
API_ENUM() enum class ClosingReason
{
    /// <summary>
    /// The unknown.
    /// </summary>
    Unknown = 0,

    /// <summary>
    /// The user.
    /// </summary>
    User,

    /// <summary>
    /// The engine exit.
    /// </summary>
    EngineExit,

    /// <summary>
    /// The close event.
    /// </summary>
    CloseEvent,
};

/// <summary>
/// Types of default cursors.
/// </summary>
API_ENUM() enum class CursorType
{
    /// <summary>
    /// The default.
    /// </summary>
    Default = 0,

    /// <summary>
    /// The cross.
    /// </summary>
    Cross,

    /// <summary>
    /// The hand.
    /// </summary>
    Hand,

    /// <summary>
    /// The help icon
    /// </summary>
    Help,

    /// <summary>
    /// The I beam.
    /// </summary>
    IBeam,

    /// <summary>
    /// The blocking image.
    /// </summary>
    No,

    /// <summary>
    /// The wait.
    /// </summary>
    Wait,

    /// <summary>
    /// The size all sides.
    /// </summary>
    SizeAll,

    /// <summary>
    /// The size NE-SW.
    /// </summary>
    SizeNESW,

    /// <summary>
    /// The size NS.
    /// </summary>
    SizeNS,

    /// <summary>
    /// The size NW-SE.
    /// </summary>
    SizeNWSE,

    /// <summary>
    /// The size WE.
    /// </summary>
    SizeWE,

    /// <summary>
    /// The cursor is hidden.
    /// </summary>
    Hidden,

    MAX
};

/// <summary>
/// Data drag and drop effects.
/// </summary>
API_ENUM() enum class DragDropEffect
{
    /// <summary>
    /// The none.
    /// </summary>
    None = 0,

    /// <summary>
    /// The copy.
    /// </summary>
    Copy,

    /// <summary>
    /// The move.
    /// </summary>
    Move,

    /// <summary>
    /// The link.
    /// </summary>
    Link,
};

/// <summary>
/// Window hit test codes. Note: they are 1:1 mapping for Win32 values.
/// </summary>
API_ENUM() enum class WindowHitCodes
{
    /// <summary>
    /// The transparent area.
    /// </summary>
    Transparent = -1,

    /// <summary>
    /// The no hit.
    /// </summary>
    NoWhere = 0,

    /// <summary>
    /// The client area.
    /// </summary>
    Client = 1,

    /// <summary>
    /// The caption area.
    /// </summary>
    Caption = 2,

    /// <summary>
    /// The system menu.
    /// </summary>
    SystemMenu = 3,

    /// <summary>
    /// The grow box
    /// </summary>
    GrowBox = 4,

    /// <summary>
    /// The menu.
    /// </summary>
    Menu = 5,

    /// <summary>
    /// The horizontal scroll.
    /// </summary>
    HScroll = 6,

    /// <summary>
    /// The vertical scroll.
    /// </summary>
    VScroll = 7,

    /// <summary>
    /// The minimize button.
    /// </summary>
    MinButton = 8,

    /// <summary>
    /// The maximize button.
    /// </summary>
    MaxButton = 9,

    /// <summary>
    /// The left side;
    /// </summary>
    Left = 10,

    /// <summary>
    /// The right side.
    /// </summary>
    Right = 11,

    /// <summary>
    /// The top side.
    /// </summary>
    Top = 12,

    /// <summary>
    /// The top left corner.
    /// </summary>
    TopLeft = 13,

    /// <summary>
    /// The top right corner.
    /// </summary>
    TopRight = 14,

    /// <summary>
    /// The bottom side.
    /// </summary>
    Bottom = 15,

    /// <summary>
    /// The bottom left corner.
    /// </summary>
    BottomLeft = 16,

    /// <summary>
    /// The bottom right corner.
    /// </summary>
    BottomRight = 17,

    /// <summary>
    /// The border.
    /// </summary>
    Border = 18,

    /// <summary>
    /// The object.
    /// </summary>
    Object = 19,

    /// <summary>
    /// The close button.
    /// </summary>
    Close = 20,

    /// <summary>
    /// The help button.
    /// </summary>
    Help = 21,
};
