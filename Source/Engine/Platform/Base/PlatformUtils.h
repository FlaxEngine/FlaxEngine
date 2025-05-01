// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/User.h"
#include "Engine/Core/Collections/Array.h"

inline void OnPlatformUserAdd(User* user)
{
    Platform::Users.Add(user);
    Platform::UserAdded(user);
}

inline void OnPlatformUserRemove(User* user)
{
    Platform::Users.Remove(user);
    Platform::UserRemoved(user);
    Delete(user);
}
