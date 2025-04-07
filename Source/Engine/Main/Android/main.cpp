// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_ANDROID

#include "Engine/Engine/Engine.h"
#include "Engine/Platform/Platform.h"
#include "android_native_app_glue.h"
#include "android_native_app_glue.c"

void android_main(android_app* app)
{
    AndroidPlatform::PreInit(app);
    Engine::Main(TEXT(""));
}

#endif
