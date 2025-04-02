// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Types/String.h"

/// <summary>
/// Specifies the initial position of a window.
/// </summary>
API_ENUM() enum class WindowStartPosition
{
    /// <summary>
    /// The window is centered within the bounds of its parent window or center screen if has no parent window specified.
    /// </summary>
    CenterParent,

    /// <summary>
    /// The windows is centered on the current display, and has the dimensions specified in the windows's size.
    /// </summary>
    CenterScreen,

    /// <summary>
    /// The position of the form is determined by the Position property.
    /// </summary>
    Manual,
};

/// <summary>
/// Specifies the type of the window.
/// </summary>
API_ENUM() enum class WindowType
{
    /// <summary>
    /// Regular window.
    /// </summary>
    Regular,

    /// <summary>
    /// Utility window.
    /// </summary>
    Utility,

    /// <summary>
    /// Tooltip window.
    /// </summary>
    Tooltip,

    /// <summary>
    /// Popup window.
    /// </summary>
    Popup,
};

/// <summary>
/// Settings for new window.
/// </summary>
API_STRUCT(NoDefault) struct CreateWindowSettings
{
DECLARE_SCRIPTING_TYPE_MINIMAL(CreateWindowSettings);

    /// <summary>
    /// The native parent window pointer.
    /// </summary>
    API_FIELD() Window* Parent = nullptr;

    /// <summary>
    /// The title.
    /// </summary>
    API_FIELD() String Title;

    /// <summary>
    /// The custom start position.
    /// </summary>
    API_FIELD() Float2 Position = Float2(100, 400);

    /// <summary>
    /// The client size.
    /// </summary>
    API_FIELD() Float2 Size = Float2(640, 480);

    /// <summary>
    /// The minimum size.
    /// </summary>
    API_FIELD() Float2 MinimumSize = Float2(1, 1);

    /// <summary>
    /// The maximum size. Set to 0 to use unlimited size.
    /// </summary>
    API_FIELD() Float2 MaximumSize = Float2(0, 0);

    /// <summary>
    /// The start position mode.
    /// </summary>
    API_FIELD() WindowStartPosition StartPosition = WindowStartPosition::Manual;

    /// <summary>
    /// True if show window fullscreen on show.
    /// </summary>
    API_FIELD() bool Fullscreen = false;

    /// <summary>
    /// Enable/disable window border.
    /// </summary>
    API_FIELD() bool HasBorder = true;

    /// <summary>
    /// Enable/disable window transparency support. Required to change window opacity property.
    /// </summary>
    API_FIELD() bool SupportsTransparency = false;

    /// <summary>
    /// True if show window on taskbar, otherwise it will be hidden.
    /// </summary>
    API_FIELD() bool ShowInTaskbar = true;

    /// <summary>
    /// Auto activate window after show.
    /// </summary>
    API_FIELD() bool ActivateWhenFirstShown = true;

    /// <summary>
    /// Allow window to capture input.
    /// </summary>
    API_FIELD() bool AllowInput = true;

    /// <summary>
    /// Allow window minimize action.
    /// </summary>
    API_FIELD() bool AllowMinimize = true;

    /// <summary>
    /// Allow window maximize action.
    /// </summary>
    API_FIELD() bool AllowMaximize = true;

    /// <summary>
    /// Enable/disable drag and drop actions over the window.
    /// </summary>
    API_FIELD() bool AllowDragAndDrop = false;

    /// <summary>
    /// True if window topmost, otherwise false as default layout.
    /// </summary>
    API_FIELD() bool IsTopmost = false;

    /// <summary>
    /// True if it's a regular window, false for tooltips, context menu and other utility windows.
    /// </summary>
    API_FIELD() DEPRECATED("Use Type instead") bool IsRegularWindow = true;

    /// <summary>
    /// The type of window. The type affects the behaviour of the window in system level.
    /// Note: Tooltip and Popup windows require Parent to be set.
    /// </summary>
    API_FIELD() WindowType Type = WindowType::Regular;

    /// <summary>
    /// Enable/disable window sizing frame.
    /// </summary>
    API_FIELD() bool HasSizingFrame = true;

    /// <summary>
    /// Enable/disable window auto-show after the first paint.
    /// </summary>
    API_FIELD() bool ShowAfterFirstPaint = true;

    /// <summary>
    /// The custom data (platform dependant).
    /// </summary>
    API_FIELD() void* Data = nullptr;
};
