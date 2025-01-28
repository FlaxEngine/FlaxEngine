// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_SDL && PLATFORM_MAC

static_assert(false, "TODO");

void SDLPlatform::SetHighDpiAwarenessEnabled(bool enable)
{
    // TODO: This is now called before Platform::Init, ensure the scaling is changed accordingly during Platform::Init (see ApplePlatform::SetHighDpiAwarenessEnabled)
}

#endif
