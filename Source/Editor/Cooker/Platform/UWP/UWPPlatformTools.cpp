// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_UWP

#include "UWPPlatformTools.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/UWP/UWPPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"

IMPLEMENT_ENGINE_SETTINGS_GETTER(UWPPlatformSettings, UWPPlatform);

const Char* UWPPlatformTools::GetDisplayName() const
{
    return TEXT("Windows Store");
}

const Char* UWPPlatformTools::GetName() const
{
    return TEXT("UWP");
}

PlatformType UWPPlatformTools::GetPlatform() const
{
    return PlatformType::UWP;
}

ArchitectureType UWPPlatformTools::GetArchitecture() const
{
    return _arch;
}

DotNetAOTModes UWPPlatformTools::UseAOT() const
{
    return DotNetAOTModes::MonoAOTDynamic;
}

bool UWPPlatformTools::OnDeployBinaries(CookingData& data)
{
    const auto platformDataPath = Globals::StartupFolder / TEXT("Source/Platforms");
    const auto uwpDataPath = platformDataPath / TEXT("UWP/Binaries");
    const auto gameSettings = GameSettings::Get();
    const auto platformSettings = UWPPlatformSettings::Get();
    StringAnsi fileTemplate;

    // Copy binaries
    const auto binPath = data.GetGameBinariesPath();
    Array<String> files;
    files.Add(binPath / TEXT("FlaxEngine.pri"));
    files.Add(binPath / TEXT("FlaxEngine.winmd"));
    files.Add(binPath / TEXT("FlaxEngine.xml"));
    FileSystem::DirectoryGetFiles(files, binPath, TEXT("*.dll"), DirectorySearchOption::TopDirectoryOnly);
    if (data.Configuration != BuildConfiguration::Release)
    {
        FileSystem::DirectoryGetFiles(files, binPath, TEXT("*.pdb"), DirectorySearchOption::TopDirectoryOnly);
    }
    for (int32 i = 0; i < files.Count(); i++)
    {
        if (!FileSystem::FileExists(files[i]))
        {
            data.Error(String::Format(TEXT("Missing source file {0}."), files[i]));
            return true;
        }

        if (FileSystem::CopyFile(data.DataOutputPath / StringUtils::GetFileName(files[i]), files[i]))
        {
            data.Error(TEXT("Failed to setup output directory."));
            return true;
        }
    }

    const auto projectName = gameSettings->ProductName;
    auto defaultNamespace = projectName;
    ScriptsBuilder::FilterNamespaceText(defaultNamespace);
    const StringAnsi projectGuid = "{3A9A2246-71DD-4567-9ABF-3E040310E30E}";
    const String productId = Guid::New().ToString(Guid::FormatType::D);
    const char* mode;
    switch (data.Platform)
    {
    case BuildPlatform::UWPx86:
        mode = "x86";
        break;
    case BuildPlatform::UWPx64:
        mode = "x64";
        break;
    default:
        return true;
    }

    // Prepare certificate
    const auto srcCertificatePath = Globals::ProjectFolder / platformSettings->CertificateLocation;
    const auto dstCertificatePath = data.DataOutputPath / TEXT("WSACertificate.pfx");
    if (platformSettings->CertificateLocation.HasChars() && FileSystem::FileExists(srcCertificatePath))
    {
        // Use cert from settings
        if (FileSystem::CopyFile(dstCertificatePath, srcCertificatePath))
        {
            data.Error(TEXT("Failed to copy WSACertificate.pfx file."));
            return true;
        }
    }
    else
    {
        // Generate new temp cert if missing
        if (!FileSystem::FileExists(dstCertificatePath))
        {
            if (EditorUtilities::GenerateCertificate(gameSettings->CompanyName, dstCertificatePath))
            {
                LOG(Warning, "Failed to create certificate.");
            }
        }
    }

    // Copy assets
    const auto dstAssetsPath = data.DataOutputPath / TEXT("Assets");
    const auto srcAssetsPath = uwpDataPath / TEXT("Assets");
    if (!FileSystem::DirectoryExists(dstAssetsPath))
    {
        if (FileSystem::CopyDirectory(dstAssetsPath, srcAssetsPath))
        {
            data.Error(TEXT("Failed to copy Assets directory."));
            return true;
        }
    }
    const auto dstPropertiesPath = data.DataOutputPath / TEXT("Properties");
    if (!FileSystem::DirectoryExists(dstPropertiesPath))
    {
        if (FileSystem::CreateDirectory(dstPropertiesPath))
        {
            data.Error(TEXT("Failed to create Properties directory."));
            return true;
        }
    }
    const auto dstDefaultRdXmlPath = dstPropertiesPath / TEXT("Default.rd.xml");
    const auto srcDefaultRdXmlPath = uwpDataPath / TEXT("Default.rd.xml");
    if (!FileSystem::FileExists(dstDefaultRdXmlPath))
    {
        if (FileSystem::CopyFile(dstDefaultRdXmlPath, srcDefaultRdXmlPath))
        {
            data.Error(TEXT("Failed to copy Default.rd.xml file."));
            return true;
        }
    }
    const auto dstAssemblyInfoPath = dstPropertiesPath / TEXT("AssemblyInfo.cs");
    const auto srcAssemblyInfoPath = uwpDataPath / TEXT("AssemblyInfo.cs");
    if (!FileSystem::FileExists(dstAssemblyInfoPath))
    {
        // Get template
        if (File::ReadAllText(srcAssemblyInfoPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load AssemblyInfo.cs template."));
            return true;
        }

        // Write data to file
        auto file = FileWriteStream::Open(dstAssemblyInfoPath);
        bool hasError = true;
        if (file)
        {
            auto now = DateTime::Now();
            file->WriteText(StringAnsi::Format(
                fileTemplate.Get()
                , gameSettings->ProductName.ToStringAnsi()
                , gameSettings->CompanyName.ToStringAnsi()
                , now.GetYear()
            ));
            hasError = file->HasError();
            Delete(file);
        }
        if (hasError)
        {
            data.Error(TEXT("Failed to create AssemblyInfo.cs."));
            return true;
        }
    }
    const auto dstAppPath = data.DataOutputPath / TEXT("App.cs");
    const auto srcAppPath = uwpDataPath / TEXT("App.cs");
    if (!FileSystem::FileExists(dstAppPath))
    {
        // Get template
        if (File::ReadAllText(srcAppPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load App.cs template."));
            return true;
        }

        // Write data to file
        auto file = FileWriteStream::Open(dstAppPath);
        bool hasError = true;
        if (file)
        {
            file->WriteText(StringAnsi::Format(
                fileTemplate.Get()
                , defaultNamespace.ToStringAnsi() // {0} Default Namespace
            ));
            hasError = file->HasError();
            Delete(file);
        }
        if (hasError)
        {
            data.Error(TEXT("Failed to create App.cs."));
            return true;
        }
    }
    const auto dstFlaxGeneratedPath = data.DataOutputPath / TEXT("FlaxGenerated.cs");
    const auto srcFlaxGeneratedPath = uwpDataPath / TEXT("FlaxGenerated.cs");
    {
        // Get template
        if (File::ReadAllText(srcFlaxGeneratedPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load FlaxGenerated.cs template."));
            return true;
        }

        // Prepare
        StringAnsi autoRotationPreferences;
        if ((int)platformSettings->AutoRotationPreferences & (int)UWPPlatformSettings::DisplayOrientations::Landscape)
        {
            autoRotationPreferences += "DisplayOrientations.Landscape";
        }
        if ((int)platformSettings->AutoRotationPreferences & (int)UWPPlatformSettings::DisplayOrientations::LandscapeFlipped)
        {
            if (autoRotationPreferences.HasChars())
                autoRotationPreferences += " | ";
            autoRotationPreferences += "DisplayOrientations.LandscapeFlipped";
        }
        if ((int)platformSettings->AutoRotationPreferences & (int)UWPPlatformSettings::DisplayOrientations::Portrait)
        {
            if (autoRotationPreferences.HasChars())
                autoRotationPreferences += " | ";
            autoRotationPreferences += "DisplayOrientations.Portrait";
        }
        if ((int)platformSettings->AutoRotationPreferences & (int)UWPPlatformSettings::DisplayOrientations::PortraitFlipped)
        {
            if (autoRotationPreferences.HasChars())
                autoRotationPreferences += " | ";
            autoRotationPreferences += "DisplayOrientations.PortraitFlipped";
        }
        StringAnsi preferredLaunchWindowingMode = platformSettings->PreferredLaunchWindowingMode == UWPPlatformSettings::WindowMode::FullScreen ? "FullScreen" : "PreferredLaunchViewSize";

        // Write data to file
        auto file = FileWriteStream::Open(dstFlaxGeneratedPath);
        bool hasError = true;
        if (file)
        {
            file->WriteText(StringAnsi::Format(
                fileTemplate.Get()
                , autoRotationPreferences.Get()
                , preferredLaunchWindowingMode.Get()
            ));
            hasError = file->HasError();
            Delete(file);
        }
        if (hasError)
        {
            data.Error(TEXT("Failed to create FlaxGenerated.cs."));
            return true;
        }
    }

    // Create solution
    const auto dstSolutionPath = data.DataOutputPath / projectName + TEXT(".sln");
    const auto srcSolutionPath = uwpDataPath / TEXT("Solution.sln");
    if (!FileSystem::FileExists(dstSolutionPath))
    {
        // Get template
        if (File::ReadAllText(srcSolutionPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load Solution.sln template."));
            return true;
        }

        // Write data to file
        auto file = FileWriteStream::Open(dstSolutionPath);
        bool hasError = true;
        if (file)
        {
            file->WriteText(StringAnsi::Format(
                fileTemplate.Get()
                , projectName.ToStringAnsi() // {0} Project Name
                , mode // {1} Platform Mode
                , projectGuid.ToStringAnsi() // {2} Project ID
            ));
            hasError = file->HasError();
            Delete(file);
        }
        if (hasError)
        {
            data.Error(TEXT("Failed to create Solution.sln."));
            return true;
        }
    }

    // Create project
    const auto dstProjectPath = data.DataOutputPath / projectName + TEXT(".csproj");
    const auto srcProjectPath = uwpDataPath / TEXT("Project.csproj");
    {
        // Get template
        if (File::ReadAllText(srcProjectPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load Project.csproj template."));
            return true;
        }

        // Build included files data
        StringBuilder filesInclude(2048);
        for (int32 i = 0; i < files.Count(); i++)
        {
            // Link dlls (except FlaxEngine.dll because it's linked as a reference)
            if (files[i].EndsWith(TEXT(".dll")) && !files[i].EndsWith(TEXT("FlaxEngine.dll")))
            {
                String filename = StringUtils::GetFileName(files[i]);
                filename.Replace(TEXT('/'), TEXT('\\'), StringSearchCase::CaseSensitive);

                filesInclude.Append(TEXT("\n    <Content Include=\""));
                filesInclude.Append(filename);
                filesInclude.Append(TEXT("\" />"));
            }
        }

        // Write data to file
        auto file = FileWriteStream::Open(dstProjectPath);
        bool hasError = true;
        if (file)
        {
            file->WriteText(StringAnsi::Format(
                fileTemplate.Get()
                , projectName.ToStringAnsi() // {0} Project Name
                , mode // {1} Platform Mode
                , projectGuid.Get() // {2} Project ID
                , filesInclude.ToString().ToStringAnsi() // {3} Files to include
                , defaultNamespace.ToStringAnsi() // {4} Default Namespace
            ));
            hasError = file->HasError();
            Delete(file);
        }
        if (hasError)
        {
            data.Error(TEXT("Failed to create Project.csproj."));
            return true;
        }
    }

    // Create manifest
    const auto dstManifestPath = data.DataOutputPath / TEXT("Package.appxmanifest");
    const auto srcManifestPath = uwpDataPath / TEXT("Package.appxmanifest");
    if (!FileSystem::FileExists(dstManifestPath))
    {
        // Get template
        if (File::ReadAllText(srcManifestPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load Package.appxmanifest template."));
            return true;
        }

        // Build included files data
        StringBuilder filesInclude(2048);
        for (int32 i = 0; i < files.Count(); i++)
        {
            if (files[i].EndsWith(TEXT(".dll")))
            {
                String filename = StringUtils::GetFileName(files[i]);
                filename.Replace(TEXT('/'), TEXT('\\'), StringSearchCase::CaseSensitive);

                filesInclude.Append(TEXT("\n    <Content Include=\""));
                filesInclude.Append(filename);
                filesInclude.Append(TEXT("\" />"));
            }
        }

        // Write data to file
        auto file = FileWriteStream::Open(dstManifestPath);
        bool hasError = true;
        if (file)
        {
            file->WriteText(StringAnsi::Format(
                fileTemplate.Get()
                , projectName.ToStringAnsi() // {0} Display Name
                , gameSettings->CompanyName.ToStringAnsi() // {1} Company Name
                , productId.ToStringAnsi() // {2} Product ID
                , defaultNamespace.ToStringAnsi() // {3} Default Namespace
            ));
            hasError = file->HasError();
            Delete(file);
        }
        if (hasError)
        {
            data.Error(TEXT("Failed to create Package.appxmanifest."));
            return true;
        }
    }

    return false;
}

bool UWPPlatformTools::OnPostProcess(CookingData& data)
{
    LOG(Error, "UWP (Windows Store) platform has been deprecated and is no longer supported");

    // Special case for UWP
    // FlaxEngine.dll cannot be added to the solution as `Content` item (due to conflicts with C++ /CX FlaxEngine.dll)
    // Use special directory for it (generated UWP project handles this case and copies lib to the output)
    const String assembliesPath = data.DataOutputPath;
    const auto dstPath1 = data.DataOutputPath / TEXT("DataSecondary");
    if (!FileSystem::DirectoryExists(dstPath1))
    {
        if (FileSystem::CreateDirectory(dstPath1))
        {
            data.Error(TEXT("Failed to create DataSecondary directory."));
            return true;
        }
    }
    if (FileSystem::MoveFile(dstPath1 / TEXT("FlaxEngine.dll"), assembliesPath / TEXT("FlaxEngine.dll"), true))
    {
        data.Error(TEXT("Failed to move FlaxEngine.dll to DataSecondary directory."));
        return true;
    }

    return false;
}

#endif
