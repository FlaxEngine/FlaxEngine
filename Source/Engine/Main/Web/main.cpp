// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WEB

#include "Engine/Engine/Engine.h"
#include <emscripten/emscripten.h>

class PlatformMain
{
    static void Loop()
    {
        // Tick engine
        Engine::OnLoop();

        if (Engine::ShouldExit())
        {
            // Exit engine
            Engine::OnExit();
            emscripten_cancel_main_loop();
            emscripten_force_exit(Engine::ExitCode);
            return;
        }
    }

public:
    static int32 Main()
    {
        // Initialize engine
        int32 result = Engine::OnInit(TEXT(""));
        if (result != 0)
            return result;

        // Setup main loop to be called by Emscripten
        emscripten_set_main_loop(Loop, -1, false);
        emscripten_set_main_loop_timing(EM_TIMING_RAF, 1); // Run main loop on each animation frame (vsync)

        // Run the first loop
        Loop();

        return 0;
    }
};

int main()
{
    return PlatformMain::Main();
}

#endif
