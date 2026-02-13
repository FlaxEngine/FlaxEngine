// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_SDL && PLATFORM_WEB

#include "SDLWindow.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Core/Log.h"
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_error.h>

bool SDLPlatform::InitInternal()
{
    return false;
}

bool SDLPlatform::UsesWindows()
{
    return false;
}

bool SDLPlatform::UsesWayland()
{
    return false;
}

bool SDLPlatform::UsesX11()
{
    return false;
}

void SDLPlatform::PreHandleEvents()
{
}

void SDLPlatform::PostHandleEvents()
{
}

bool SDLWindow::HandleEventInternal(SDL_Event& event)
{
    return false;
}

void SDLPlatform::SetHighDpiAwarenessEnabled(bool enable)
{
}

DragDropEffect SDLWindow::DoDragDrop(const StringView& data)
{
    return DragDropEffect::None;
}

DragDropEffect SDLWindow::DoDragDrop(const StringView& data, const Float2& offset, Window* dragSourceWindow)
{
    Show();
    return DragDropEffect::None;
}

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    StringAnsi textAnsi(text);
    StringAnsi captionAnsi(caption);

    SDL_MessageBoxData data;
    SDL_MessageBoxButtonData dataButtons[3];
    data.window = nullptr;
    data.title = captionAnsi.GetText();
    data.message = textAnsi.GetText();
    data.colorScheme = nullptr;

    switch (icon)
    {
    case MessageBoxIcon::Error:
    case MessageBoxIcon::Hand:
    case MessageBoxIcon::Stop:
        data.flags |= SDL_MESSAGEBOX_ERROR;
        break;
    case MessageBoxIcon::Asterisk:
    case MessageBoxIcon::Information:
    case MessageBoxIcon::Question:
        data.flags |= SDL_MESSAGEBOX_INFORMATION;
        break;
    case MessageBoxIcon::Exclamation:
    case MessageBoxIcon::Warning:
        data.flags |= SDL_MESSAGEBOX_WARNING;
        break;
    default:
        break;
    }

    switch (buttons)
    {
    case MessageBoxButtons::AbortRetryIgnore:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Abort,
            "Abort"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Retry,
            "Retry"
        };
        dataButtons[2] =
        {
            0,
            (int)DialogResult::Ignore,
            "Ignore"
        };
        data.numbuttons = 3;
        break;
    case MessageBoxButtons::OK:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::OK,
            "OK"
        };
        data.numbuttons = 1;
        break;
    case MessageBoxButtons::OKCancel:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::OK,
            "OK"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Cancel,
            "Cancel"
        };
        data.numbuttons = 2;
        break;
    case MessageBoxButtons::RetryCancel:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Retry,
            "Retry"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Cancel,
            "Cancel"
        };
        data.numbuttons = 2;
        break;
    case MessageBoxButtons::YesNo:
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Yes,
            "Yes"
        };
        dataButtons[1] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::No,
            "No"
        };
        data.numbuttons = 2;
        break;
    case MessageBoxButtons::YesNoCancel:
    {
        dataButtons[0] =
        {
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            (int)DialogResult::Yes,
            "Yes"
        };
        dataButtons[1] =
        {
            0,
            (int)DialogResult::No,
            "No"
        };
        dataButtons[2] =
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            (int)DialogResult::Cancel,
            "Cancel"
        };
        data.numbuttons = 3;
        break;
    }
    default:
        break;
    }
    data.buttons = dataButtons;

    int result = -1;
    if (!SDL_ShowMessageBox(&data, &result))
    {
        LOG(Error, "Failed to show SDL message box: {0}", String(SDL_GetError()));
        return DialogResult::Abort;
    }
    if (result < 0)
        return DialogResult::None;
    return (DialogResult)result;
}

#endif
