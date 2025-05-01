// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_GDK

#include "Engine/Platform/Base/UserBase.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include <XGameRuntime.h>

/// <summary>
/// Implementation of the user for GDK platform.
/// </summary>
class FLAXENGINE_API GDKUser : public UserBase
{
public:

    GDKUser(XUserHandle userHandle, XUserLocalId localId, const String& name)
        : UserBase(name)
    {
        UserHandle = userHandle;
        LocalId = localId;
    }

    ~GDKUser()
    {
        XUserCloseHandle(UserHandle);
    }

public:

    XUserHandle UserHandle;
    XUserLocalId LocalId;
    Array<APP_LOCAL_DEVICE_ID, FixedAllocation<32>> AssociatedDevices;
};

#endif
