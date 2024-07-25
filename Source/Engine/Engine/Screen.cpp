// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Screen.h"
#include "Engine.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Nullable.h"
#include "Engine/Platform/Window.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/Managed/ManagedEditor.h"
#else
#include "Engine/Engine/Engine.h"
#endif

namespace
{
    Nullable<bool> Fullscreen;
    Nullable<Float2> Size;
    bool CursorVisible = true;
    CursorLockMode CursorLock = CursorLockMode::None;
    bool LastGameViewportFocus = false;
}

class ScreenService : public EngineService
{
public:
    ScreenService()
        : EngineService(TEXT("Screen"), 500)
    {
    }

    void Update() override;
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

Float2 Screen::GetSize()
{
#if USE_EDITOR
    return Editor::Managed->GetGameWindowSize();
#else
	auto win = Engine::MainWindow;
	return win ? win->GetClientSize() : Float2::Zero;
#endif
}

void Screen::SetSize(const Float2& value)
{
    if (value.X <= 0 || value.Y <= 0)
    {
        LOG(Error, "Invalid Screen size to set.");
        return;
    }

    Size = value;
}

Float2 Screen::ScreenToGameViewport(const Float2& screenPos)
{
#if USE_EDITOR
    return Editor::Managed->ScreenToGameViewport(screenPos);
#else
    auto win = Engine::MainWindow;
    return win ? win->ScreenToClient(screenPos) : Float2::Minimum;
#endif
}

Float2 Screen::GameViewportToScreen(const Float2& viewportPos)
{
#if USE_EDITOR
    return Editor::Managed->GameViewportToScreen(viewportPos);
#else
    auto win = Engine::MainWindow;
    return win ? win->ClientToScreen(viewportPos) : Float2::Minimum;
#endif
}

bool Screen::GetCursorVisible()
{
    return CursorVisible;
}

void Screen::SetCursorVisible(const bool value)
{
#if USE_EDITOR
    const auto win = Editor::Managed->GetGameWindow(true);
#else
	const auto win = Engine::MainWindow;
#endif
    if (win && Engine::HasGameViewportFocus())
    {
        win->SetCursor(value ? CursorType::Default : CursorType::Hidden);
    }
    else if (win)
        win->SetCursor(CursorType::Default);
    CursorVisible = value;
}

CursorLockMode Screen::GetCursorLock()
{
    return CursorLock;
}

void Screen::SetCursorLock(CursorLockMode mode)
{
#if USE_EDITOR
    const auto win = Editor::Managed->GetGameWindow(true);
#else
    const auto win = Engine::MainWindow;
#endif
    if (win && mode == CursorLockMode::Clipped)
    {
#if USE_EDITOR
        Rectangle bounds(Editor::Managed->GameViewportToScreen(Float2::Zero), Editor::Managed->GetGameWindowSize());
#else
        Rectangle bounds = win->GetClientBounds();
#endif
        win->StartClippingCursor(bounds);
    }
    else if (win && CursorLock == CursorLockMode::Clipped)
    {
        win->EndClippingCursor();
    }
    CursorLock = mode;
}

GameWindowMode Screen::GetGameWindowMode()
{
    GameWindowMode result = GameWindowMode::Windowed;
#if !USE_EDITOR
    auto win = Engine::MainWindow;
    if (win)
    {
        if (GetIsFullscreen() || win->IsFullscreen())
            result = GameWindowMode::Fullscreen;
        else if (win->GetSettings().HasBorder)
            result = GameWindowMode::Windowed;
        else if (win->GetClientPosition().IsZero() && win->GetSize() == Platform::GetDesktopSize())
            result = GameWindowMode::FullscreenBorderless;
        else
            result = GameWindowMode::Borderless;
    }
#endif
    return result;
}

void Screen::SetGameWindowMode(GameWindowMode windowMode)
{
#if !USE_EDITOR
    auto win = Engine::MainWindow;
    if (!win)
        return;
    switch (windowMode)
    {
    case GameWindowMode::Windowed:
        if (GetIsFullscreen())
            SetIsFullscreen(false);
        win->SetBorderless(false, false);
        break;
    case GameWindowMode::Fullscreen:
        SetIsFullscreen(true);
        break;
    case GameWindowMode::Borderless:
        win->SetBorderless(true, false);
        break;
    case GameWindowMode::FullscreenBorderless:
        win->SetBorderless(true, true);
        break;
    }
#endif
}

Window* Screen::GetMainWindow()
{
    return Engine::MainWindow;
}

void ScreenService::Update()
{
#if USE_EDITOR
    // Sync current cursor state in Editor (eg. when viewport focus can change)
    const auto win = Editor::Managed->GetGameWindow(true);
    bool gameViewportFocus = win && Engine::HasGameViewportFocus();
    if (gameViewportFocus != LastGameViewportFocus)
        Screen::SetCursorVisible(CursorVisible);
    LastGameViewportFocus = gameViewportFocus;
#endif
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
