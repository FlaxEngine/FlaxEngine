// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "Screen.h"
#include "Engine.h"
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
Nullable<Float2> Size;
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

Array<Float2> Screen::GetAllResolutions() {

    // reference: https://en.wikipedia.org/wiki/List_of_common_resolutions
#if PLATFORM_DESKTOP || PLATFORM_LINUX
    // all list of monitors and video resolutions
    Float2 rawResolutions[66] = {
        Float2(480, 480),
        Float2(528, 480),
        Float2(544, 576),
        Float2(544, 480),
        Float2(640, 480),
        Float2(704, 480),
        Float2(704, 576),
        Float2(720, 480),
        Float2(720, 486),
        Float2(720, 576),
        Float2(768, 480),
        Float2(800, 480),
        Float2(800, 600),
        Float2(960, 720),
        Float2(1024, 600),
        Float2(1024, 768),
        Float2(1024, 1024),
        Float2(1280, 720),
        Float2(1280, 800),
        Float2(1280, 1024),
        Float2(1280, 1080),
        Float2(1366, 768),
        Float2(1440, 900),
        Float2(1440, 1024),
        Float2(1440, 1080),
        Float2(1600, 900),
        Float2(1600, 1024),
        Float2(1600, 1200),
        Float2(1680, 1050),
        Float2(1828, 1332),
        Float2(1920, 1080),
        Float2(1920, 1400),
        Float2(1920, 1440),
        Float2(1990, 1080),
        Float2(2048, 858),
        Float2(2048, 1080),
        Float2(2048, 1280),
        Float2(2048, 1440),
        Float2(2048, 1536),
        Float2(2304, 1728),
        Float2(2400, 1080),
        Float2(2560, 1080),
        Float2(2560, 1440),
        Float2(2560, 1600),
        Float2(2800, 2100),
        Float2(3840, 2160),
        Float2(2048, 1556),
        Float2(3000, 2000),
        Float2(3240, 2160),
        Float2(3840, 2160),
        Float2(4096, 1714),
        Float2(4096, 2304),
        Float2(5120, 2160),
        Float2(5120, 2880),
        Float2(5120, 1440),
        Float2(5120, 3200),
        Float2(6012, 3384),
        Float2(7200, 3600),
        Float2(7680, 4320),
        Float2(7680, 4320),
        Float2(8192, 4320),
        Float2(10240, 4320),
        Float2(15360, 8640)
    };

    Float2 primaryResolution = Platform::GetDesktopSize();
    Array<Float2> resolutions = Array<Float2>();

    // invert horizontal resolutions to vertical
    if (primaryResolution.Y > primaryResolution.X)
    {
        for (int i = 0; i < rawResolutions->Length(); i++)
        {
            rawResolutions[i] = Float2(rawResolutions[i].Y, rawResolutions[i].Y);
        }
    }

    for each (Float2 r in rawResolutions)
    {
        if (r.X >= primaryResolution.X && r.Y >= primaryResolution.Y)
        {
            break;
        }

        resolutions.Add(r);
    }

    resolutions.Add(primaryResolution);
    return resolutions;
#else
    return Array<Float2>({ primaryResolution });
#endif
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
#if USE_EDITOR
    const auto win = Editor::Managed->GetGameWindow(true);
#else
	const auto win = Engine::MainWindow;
#endif
    return win ? win->GetCursor() != CursorType::Hidden : true;
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
