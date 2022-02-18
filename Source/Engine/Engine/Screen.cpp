// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "Screen.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Nullable.h"
#include "Engine/Platform/Window.h"
#include "Engine/Engine/EngineService.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/Managed/ManagedEditor.h"
#else
#include "Engine/Engine/Engine.h"
#endif

Nullable<bool> Fullscreen;
Nullable<Vector2> Size;
static CursorLockMode CursorLock = CursorLockMode::None;

class ScreenService : public EngineService
{
public:

    ScreenService()
        : EngineService(TEXT("Screen"), 120)
    {
    }

    void Draw() override;
};

ScreenService ScreenServiceInstance;

bool Screen::GetIsFullscreen()
{
#if USE_EDITOR
    return false;
#else
	auto win = Engine::MainWindow;
	return win ? win->IsFullscreen() : false;
#endif
}

void Screen::SetIsFullscreen(bool value)
{
    Fullscreen = value;
}

Vector2 Screen::GetSize()
{
#if USE_EDITOR
    return Editor::Managed->GetGameWindowSize();
#else
	auto win = Engine::MainWindow;
	return win ? win->GetClientSize() : Vector2::Zero;
#endif
}

void Screen::SetSize(const Vector2& value)
{
    if (value.X <= 0 || value.Y <= 0)
    {
        LOG(Error, "Invalid Screen size to set.");
        return;
    }

    Size = value;
}

Vector2 Screen::ScreenToGameViewport(const Vector2& screenPos)
{
#if USE_EDITOR
    return Editor::Managed->ScreenToGameViewport(screenPos);
#else
    auto win = Engine::MainWindow;
    return win ? win->ScreenToClient(screenPos) : Vector2::Minimum;
#endif
}

Vector2 Screen::GameViewportToScreen(const Vector2& viewportPos)
{
#if USE_EDITOR
    return Editor::Managed->GameViewportToScreen(viewportPos);
#else
    auto win = Engine::MainWindow;
    return win ? win->ClientToScreen(viewportPos) : Vector2::Minimum;
#endif
}

bool Screen::GetCursorVisible()
{
#if USE_EDITOR
    const auto win = Editor::Managed->GetGameWindow();
#else
	const auto win = Engine::MainWindow;
#endif
    return win ? win->GetCursor() != CursorType::Hidden : true;
}

void Screen::SetCursorVisible(const bool value)
{
#if USE_EDITOR
    const auto win = Editor::Managed->GetGameWindow();
#else
	const auto win = Engine::MainWindow;
#endif
    if (win)
    {
        win->SetCursor(value ? CursorType::Default : CursorType::Hidden);
    }
}

CursorLockMode Screen::GetCursorLock()
{
    return CursorLock;
}

void Screen::SetCursorLock(CursorLockMode mode)
{
#if USE_EDITOR
    const auto win = Editor::Managed->GetGameWindow();
#else
    const auto win = Engine::MainWindow;
#endif
    if (win && mode == CursorLockMode::Clipped)
    {
        win->StartClippingCursor(win->GetClientBounds());
    }
    else if (win && CursorLock == CursorLockMode::Clipped)
    {
        win->EndClippingCursor();
    }
    CursorLock = mode;
}

void ScreenService::Draw()
{
#if USE_EDITOR

    // Not supported

#else

	if (Fullscreen.HasValue())
	{
		auto win = Engine::MainWindow;
		if (win)
		{
			win->SetIsFullscreen(Fullscreen.GetValue());
		}

		Fullscreen.Reset();
	}

	if (Size.HasValue())
	{
		auto win = Engine::MainWindow;
		if (win)
		{
			win->SetClientSize(Size.GetValue());
		}

		Size.Reset();
	}

#endif
}
