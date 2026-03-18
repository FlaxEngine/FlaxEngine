// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WEB

#include "Engine/Engine/Engine.h"
#include <emscripten/emscripten.h>

// Reference: https://github.com/kainino0x/webgpu-cross-platform-demo/blob/f5c69c6fccbb2584c1b6f9e559f9a41a38a9b5ad/main.cpp#L692-L704
// Reference: https://github.com/kainino0x/webgpu-cross-platform-demo/blob/c26ea3e29ed9f73f9b39bddf7964b482ce3c6964/main.cpp#L737-L758
#define WEB_LOOP_MODE 2 // 0 - default, 1 - Asyncify, 2 - JSPI
#if WEB_LOOP_MODE != 0
// Workaround for JSPI not working in emscripten_set_main_loop. Loosely based on this code:
// https://github.com/emscripten-core/emscripten/issues/22493#issuecomment-2330275282
// This code only works with JSPI is enabled.
typedef bool (*FrameCallback)(); // If callback returns true, continues the loop.
EM_JS(void, requestAnimationFrameLoopWithJSPI, (FrameCallback callback), {
#if WEB_LOOP_MODE == 2
    var callback = WebAssembly.promising(getWasmTableEntry(callback));
#elif WEB_LOOP_MODE == 1
    var callback = () = > globalThis['Module']['ccall']("callback", "boolean", [], [], { async: true });
#endif
    async function tick() {
        // Start the frame callback. 'await' means we won't call
        // requestAnimationFrame again until it completes.
        var keepLooping = await callback();
        if (keepLooping) requestAnimationFrame(tick);
    }
    requestAnimationFrame(tick);
    })
#endif

class PlatformMain
{
#if WEB_LOOP_MODE == 0
    static void Loop()
    {
        // Tick engine
        Engine::OnLoop();

        if (Engine::ShouldExit())
        {
            // Exit engine
            Engine::OnExit();
            return;
        }
    }
#else
    static bool Loop()
    {
        if (Engine::FatalError != FatalErrorType::None)
            return false;

        // Tick engine
        Engine::OnLoop();

        if (Engine::ShouldExit())
        {
            // Exit engine
            Engine::OnExit();
            emscripten_cancel_main_loop();
            emscripten_force_exit(Engine::ExitCode);
            return false;
        }

        return true;
    }
#endif

public:
    static int32 Main()
    {
        // Initialize engine
        int32 result = Engine::OnInit(TEXT(""));
        if (result != 0)
            return result;

        // Setup main loop to be called by Emscripten
#if WEB_LOOP_MODE == 0
        emscripten_set_main_loop(Loop, -1, false);
#else
        requestAnimationFrameLoopWithJSPI(Loop);
#endif
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
