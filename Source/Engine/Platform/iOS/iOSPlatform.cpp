// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if PLATFORM_IOS

#include "iOSPlatform.h"
#include "iOSWindow.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/MessageBox.h"
#include <UIKit/UIKit.h>
#include <sys/utsname.h>

int32 Dpi = 96;
Guid DeviceId;

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    // TODO: implement message box popup on iOS
    return DialogResult::OK;
}

bool iOSPlatform::Init()
{
    if (ApplePlatform::Init())
        return true;

    ScreenScale = [[UIScreen mainScreen] scale];
    CustomDpiScale *= ScreenScale;
    Dpi = 72; // TODO: calculate screen dpi (probably hardcoded map for iPhone model)

    return false;
}

void iOSPlatform::LogInfo()
{
    ApplePlatform::LogInfo();

	struct utsname systemInfo;
	uname(&systemInfo);
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    LOG(Info, "{3}, iOS {0}.{1}.{2}", version.majorVersion, version.minorVersion, version.patchVersion, String(systemInfo.machine));
}

void iOSPlatform::Tick()
{
    // Process system events
	while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0001, true) == kCFRunLoopRunHandledSource)
    {
    }
}

int32 iOSPlatform::GetDpi()
{
    return Dpi;
}

Guid iOSPlatform::GetUniqueDeviceId()
{
    return Guid::Empty; // TODO: use MAC address of the iPhone to generate device id (at least within network connection state)
}

String iOSPlatform::GetComputerName()
{
    return TEXT("iPhone");
}

Float2 iOSPlatform::GetDesktopSize()
{
    CGRect frame = [[UIScreen mainScreen] bounds];
	float scale = [[UIScreen mainScreen] scale];
    return Float2((float)frame.size.width * scale, (float)frame.size.height * scale);
}

String iOSPlatform::GetMainDirectory()
{
    String path = StringUtils::GetDirectoryName(GetExecutableFilePath());
    if (path.EndsWith(TEXT("/Contents/iOS")))
    {
        // If running from executable in a package, go up to the Contents
        path = StringUtils::GetDirectoryName(path);
    }
    return path;
}

Window* iOSPlatform::CreateWindow(const CreateWindowSettings& settings)
{
    return New<iOSWindow>(settings);
}

#endif
