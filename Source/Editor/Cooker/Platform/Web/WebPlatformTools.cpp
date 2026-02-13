// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_WEB

#include "WebPlatformTools.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Web/WebPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

IMPLEMENT_SETTINGS_GETTER(WebPlatformSettings, WebPlatform);

const Char* WebPlatformTools::GetDisplayName() const
{
    return TEXT("Web");
}

const Char* WebPlatformTools::GetName() const
{
    return TEXT("Web");
}

PlatformType WebPlatformTools::GetPlatform() const
{
    return PlatformType::Web;
}

ArchitectureType WebPlatformTools::GetArchitecture() const
{
    return ArchitectureType::x86;
}

DotNetAOTModes WebPlatformTools::UseAOT() const
{
    return DotNetAOTModes::MonoAOTStatic;
}

PixelFormat WebPlatformTools::GetTextureFormat(CookingData& data, TextureBase* texture, PixelFormat format)
{
    // TODO: texture compression for Web (eg. ASTC for mobile and BC for others?)
    return PixelFormatExtensions::FindUncompressedFormat(format);
}

bool WebPlatformTools::IsNativeCodeFile(CookingData& data, const String& file)
{
    String extension = FileSystem::GetExtension(file);
    return extension.IsEmpty() || extension == TEXT("html") || extension == TEXT("js") || extension == TEXT("wams");
}

bool WebPlatformTools::OnPostProcess(CookingData& data)
{
    // TODO: customizable HTML templates
    return false;
}

#endif
