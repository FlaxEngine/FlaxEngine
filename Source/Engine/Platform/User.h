// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_XBOX_ONE || PLATFORM_XBOX_SCARLETT
#include "GDK/GDKUser.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4User.h"
#elif PLATFORM_PS5
#include "Platforms/PS5/Engine/Platform/PS5User.h"
#else
#include "Base/UserBase.h"
#endif

#include "Types.h"
