// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/StringView.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Specifies identifiers to indicate the return value of a dialog box.
/// </summary>
API_ENUM() enum class DialogResult
{
    /// <summary>
    /// The abort.
    /// </summary>
    Abort = 0,

    /// <summary>
    /// The cancel.
    /// </summary>
    Cancel,

    /// <summary>
    /// The ignore.
    /// </summary>
    Ignore,

    /// <summary>
    /// The no.
    /// </summary>
    No,

    /// <summary>
    /// The none.
    /// </summary>
    None,

    /// <summary>
    /// The ok.
    /// </summary>
    OK,

    /// <summary>
    /// The retry.
    /// </summary>
    Retry,

    /// <summary>
    /// The yes.
    /// </summary>
    Yes,
};

/// <summary>
/// Specifies constants defining which information to display.
/// </summary>
API_ENUM() enum class MessageBoxIcon
{
    /// <summary>
    /// Asterisk
    /// </summary>
    Asterisk,

    /// <summary>
    /// Error
    /// </summary>
    Error,

    /// <summary>
    /// Exclamation
    /// </summary>
    Exclamation,

    /// <summary>
    /// Hand
    /// </summary>
    Hand,

    /// <summary>
    /// Information
    /// </summary>
    Information,

    /// <summary>
    /// None
    /// </summary>
    None,

    /// <summary>
    /// Question
    /// </summary>
    Question,

    /// <summary>
    /// Stop
    /// </summary>
    Stop,

    /// <summary>
    /// Warning
    /// </summary>
    Warning,
};

/// <summary>
/// Specifies constants defining which buttons to display on a Message Box.
/// </summary>
API_ENUM() enum class MessageBoxButtons
{
    /// <summary>
    /// Abort, Retry, Ignore
    /// </summary>
    AbortRetryIgnore,

    /// <summary>
    /// OK
    /// </summary>
    OK,

    /// <summary>
    /// OK, Cancel
    /// </summary>
    OKCancel,

    /// <summary>
    /// Retry, Cancel
    /// </summary>
    RetryCancel,

    /// <summary>
    /// Yes, No
    /// </summary>
    YesNo,

    /// <summary>
    /// Yes, No, Cancel
    /// </summary>
    YesNoCancel,
};

/// <summary>
/// Message dialogs utility (native platform).
/// </summary>
API_CLASS(Static) class MessageBox
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(MessageBox);

    /// <summary>
    /// Displays a message box with specified text.
    /// </summary>
    /// <param name="text">The text to display in the message box.</param>
    /// <returns>The message box dialog result.</returns>
    API_FUNCTION() FORCE_INLINE static DialogResult Show(const StringView& text)
    {
        return Show(nullptr, text, TEXT("Info"), MessageBoxButtons::OK, MessageBoxIcon::None);
    }

    /// <summary>
    /// Displays a message box with specified text and caption.
    /// </summary>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <returns>The message box dialog result.</returns>
    API_FUNCTION() FORCE_INLINE static DialogResult Show(const StringView& text, const StringView& caption)
    {
        return Show(nullptr, text, caption, MessageBoxButtons::OK, MessageBoxIcon::None);
    }

    /// <summary>
    /// Displays a message box with specified text, caption, buttons, and icon.
    /// </summary>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the MessageBoxButtons values that specifies which buttons to display in the message box.</param>
    /// <returns>The message box dialog result.</returns>
    API_FUNCTION() FORCE_INLINE static DialogResult Show(const StringView& text, const StringView& caption, MessageBoxButtons buttons)
    {
        return Show(nullptr, text, caption, buttons, MessageBoxIcon::None);
    }

    /// <summary>
    /// Displays a message box with specified text, caption, buttons, and icon.
    /// </summary>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the MessageBoxButtons values that specifies which buttons to display in the message box.</param>
    /// <param name="icon">One of the MessageBoxIcon values that specifies which icon to display in the message box.</param>
    /// <returns>The message box dialog result.</returns>
    API_FUNCTION() FORCE_INLINE static DialogResult Show(const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
    {
        return Show(nullptr, text, caption, buttons, icon);
    }

    /// <summary>
    /// Displays a message box with specified text.
    /// </summary>
    /// <param name="parent">The parent window or null if not used.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <returns>The message box dialog result.</returns>
    API_FUNCTION() FORCE_INLINE static DialogResult Show(Window* parent, const StringView& text)
    {
        return Show(parent, *text, TEXT("Info"), MessageBoxButtons::OK, MessageBoxIcon::None);
    }

    /// <summary>
    /// Displays a message box with specified text and caption.
    /// </summary>
    /// <param name="parent">The parent window or null if not used.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <returns>The message box dialog result.</returns>
    API_FUNCTION() FORCE_INLINE static DialogResult Show(Window* parent, const StringView& text, const StringView& caption)
    {
        return Show(parent, *text, *caption, MessageBoxButtons::OK, MessageBoxIcon::None);
    }

    /// <summary>
    /// Displays a message box with specified text, caption and buttons.
    /// </summary>
    /// <param name="parent">The parent window or null if not used.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the MessageBoxButtons values that specifies which buttons to display in the message box.</param>
    /// <returns>The message box dialog result.</returns>
    API_FUNCTION() FORCE_INLINE static DialogResult Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons)
    {
        return Show(parent, text, caption, buttons, MessageBoxIcon::None);
    }

    /// <summary>
    /// Displays a message box with specified text, caption, buttons, and icon.
    /// </summary>
    /// <param name="parent">The parent window or null if not used.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the MessageBoxButtons values that specifies which buttons to display in the message box.</param>
    /// <param name="icon">One of the MessageBoxIcon values that specifies which icon to display in the message box.</param>
    /// <returns>The message box dialog result.</returns>
    API_FUNCTION() static DialogResult Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon);
};
