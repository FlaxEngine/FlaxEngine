// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Coroutine.h"

void Coroutine::Example()
{
    constexpr CoroutineSuspensionPointIndex index = CoroutineSuspensionPointIndex::Update;
    constexpr CoroutineSuspensionPointsFlags flags = CoroutineSuspensionPointsFlags::Update;
}
