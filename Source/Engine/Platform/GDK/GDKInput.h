// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_GDK

/// <summary>
/// GDK platform specific implementation of the input system parts.
/// </summary>
class GDKInput
{
public:

    static void Init();
    static void Exit();
    static void Update();
};

#endif
