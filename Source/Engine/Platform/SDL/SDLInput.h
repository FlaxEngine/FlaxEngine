// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_SDL

class SDLWindow;
union SDL_Event;

/// <summary>
/// SDL specific implementation of the input system parts.
/// </summary>
class SDLInput
{
public:

    static void Init();
    static void Update();
    static bool HandleEvent(SDLWindow* window, SDL_Event& event);
};

#endif
