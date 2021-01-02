// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_ANDROID

#include "Engine/Engine/Engine.h"
#include "Engine/Platform/Platform.h"
#include <../../../../../../../sources/android/native_app_glue/android_native_app_glue.h>
#include <../../../../../../../sources/android/native_app_glue/android_native_app_glue.c>

void android_main(android_app* app)
{
    AndroidPlatform::PreInit(app);
    Engine::Main(TEXT(""));
}

#endif
