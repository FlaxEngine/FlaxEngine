// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_MAC

#include "MacPlatformTools.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Mac/MacPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"

IMPLEMENT_SETTINGS_GETTER(MacPlatformSettings, MacPlatform);

MacPlatformTools::MacPlatformTools(ArchitectureType arch)
    : _arch(arch)
{
}

const Char* MacPlatformTools::GetDisplayName() const
{
    return TEXT("Mac");
}

const Char* MacPlatformTools::GetName() const
{
    return TEXT("Mac");
}

PlatformType MacPlatformTools::GetPlatform() const
{
    return PlatformType::Mac;
}

ArchitectureType MacPlatformTools::GetArchitecture() const
{
    return _arch;
}

void MacPlatformTools::OnBuildStarted(CookingData& data)
{
    // Adjust the cooking output folders for packaging app
    const auto gameSettings = GameSettings::Get();
    String productName = gameSettings->ProductName;
    productName.Replace(TEXT(" "), TEXT(""));
    productName.Replace(TEXT("."), TEXT(""));
    productName.Replace(TEXT("-"), TEXT(""));
    String contents = productName + TEXT(".app/Contents/");
    data.DataOutputPath /= contents;
    data.NativeCodeOutputPath /= contents;
    data.ManagedCodeOutputPath /= contents;

    PlatformTools::OnBuildStarted(data);
}

bool MacPlatformTools::OnPostProcess(CookingData& data)
{
    const auto gameSettings = GameSettings::Get();
    const auto platformSettings = MacPlatformSettings::Get();
    const auto platformDataPath = data.GetPlatformBinariesRoot();
    const auto projectVersion = Editor::Project->Version.ToString();

    // Setup package name (eg. com.company.project)
    String appIdentifier = platformSettings->AppIdentifier;
    {
        String productName = gameSettings->ProductName;
        productName.Replace(TEXT(" "), TEXT(""));
        productName.Replace(TEXT("."), TEXT(""));
        productName.Replace(TEXT("-"), TEXT(""));
        String companyName = gameSettings->CompanyName;
        companyName.Replace(TEXT(" "), TEXT(""));
        companyName.Replace(TEXT("."), TEXT(""));
        companyName.Replace(TEXT("-"), TEXT(""));
        appIdentifier.Replace(TEXT("${PROJECT_NAME}"), *productName, StringSearchCase::IgnoreCase);
        appIdentifier.Replace(TEXT("${COMPANY_NAME}"), *companyName, StringSearchCase::IgnoreCase);
        appIdentifier = appIdentifier.ToLower();
        for (int32 i = 0; i < appIdentifier.Length(); i++)
        {
            const auto c = appIdentifier[i];
            if (c != '_' && c != '.' && !StringUtils::IsAlnum(c))
            {
                LOG(Error, "Apple app identifier \'{0}\' contains invalid character. Only letters, numbers, dots and underscore characters are allowed.", appIdentifier);
                return true;
            }
        }
        if (appIdentifier.IsEmpty())
        {
            LOG(Error, "Apple app identifier is empty.", appIdentifier);
            return true;
        }
    }

    return false;
}

#endif
