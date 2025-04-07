// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_GDK

#include "GDKPlatformTools.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/GDK/GDKPlatformSettings.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Graphics/PixelFormat.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Utilities/EditorUtilities.h"

GDKPlatformTools::GDKPlatformTools()
{
    // Find GDK
    Platform::GetEnvironmentVariable(TEXT("GameDKLatest"), _gdkPath);
    if (_gdkPath.IsEmpty() || !FileSystem::DirectoryExists(_gdkPath))
    {
        _gdkPath.Clear();
        Platform::GetEnvironmentVariable(TEXT("GRDKLatest"), _gdkPath);
        if (_gdkPath.IsEmpty() || !FileSystem::DirectoryExists(_gdkPath))
        {
            _gdkPath.Clear();
        }
        else
        {
            if (_gdkPath.EndsWith(TEXT("GRDK\\")))
                _gdkPath.Remove(_gdkPath.Length() - 6);
            else if (_gdkPath.EndsWith(TEXT("GRDK")))
                _gdkPath.Remove(_gdkPath.Length() - 5);
        }
    }
}

DotNetAOTModes GDKPlatformTools::UseAOT() const
{
    return DotNetAOTModes::MonoAOTDynamic;
}

bool GDKPlatformTools::OnDeployBinaries(CookingData& data)
{
    // Copy binaries
    const Char* executableFilename = TEXT("FlaxGame.exe");
    const auto binPath = data.GetGameBinariesPath();
    FileSystem::CreateDirectory(data.NativeCodeOutputPath);
    Array<String> files;
    files.Add(binPath / executableFilename);
    if (!FileSystem::FileExists(files[0]))
    {
        data.Error(String::Format(TEXT("Missing executable file ({0})."), files[0]));
        return true;
    }
    FileSystem::DirectoryGetFiles(files, binPath, TEXT("*.dll"), DirectorySearchOption::TopDirectoryOnly);
    for (int32 i = 0; i < files.Count(); i++)
    {
        if (FileSystem::CopyFile(data.NativeCodeOutputPath / StringUtils::GetFileName(files[i]), files[i]))
        {
            data.Error(String::Format(TEXT("Failed to setup output directory (file {0})."), files[i]));
            return true;
        }
    }

    return false;
}

bool GDKPlatformTools::OnPostProcess(CookingData& data, GDKPlatformSettings* platformSettings)
{
    // Configuration
    const auto gameSettings = GameSettings::Get();
    const auto project = Editor::Project;
    const Char* executableFilename = TEXT("FlaxGame.exe");
    const String assetsFolder = data.DataOutputPath / TEXT("Assets");
    if (!FileSystem::DirectoryExists(assetsFolder))
        FileSystem::CreateDirectory(assetsFolder);

    // Generate application icons
    data.StepProgress(TEXT("Deploying icons"), 0);
    if (EditorUtilities::ExportApplicationImage(platformSettings->Square150x150Logo, 150, 150, PixelFormat::B8G8R8A8_UNorm, assetsFolder / TEXT("Square150x150Logo.png")))
        return true;
    if (EditorUtilities::ExportApplicationImage(platformSettings->Square480x480Logo, 480, 480, PixelFormat::B8G8R8A8_UNorm, assetsFolder / TEXT("Square480x480Logo.png")))
        return true;
    if (EditorUtilities::ExportApplicationImage(platformSettings->Square44x44Logo, 44, 44, PixelFormat::B8G8R8A8_UNorm, assetsFolder / TEXT("Square44x44Logo.png")))
        return true;
    if (EditorUtilities::ExportApplicationImage(platformSettings->StoreLogo, 100, 100, PixelFormat::B8G8R8A8_UNorm, assetsFolder / TEXT("StoreLogo.png")))
        return true;
    if (EditorUtilities::ExportApplicationImage(platformSettings->SplashScreenImage, 1920, 1080, PixelFormat::B8G8R8X8_UNorm, assetsFolder / TEXT("SplashScreenImage.png"), EditorUtilities::ApplicationImageType::SplashScreen))
        return true;

    // Generate MicrosoftGame.config file
    data.StepProgress(TEXT("Generating package meta"), 0.2f);
    const String configFilePath = data.DataOutputPath / TEXT("MicrosoftGame.config");
    LOG(Info, "Generating config file to \"{0}\"", configFilePath);
    {
        // Process name to be valid
        StringBuilder sb;
        Array<Char> validName;
        const String& name = platformSettings->Name.HasChars() ? platformSettings->Name : gameSettings->ProductName;
        for (int32 i = 0; i < name.Length() && validName.Count() <= 50; i++)
        {
            auto c = name[i];
            switch (c)
            {
            case '.':
            case '[':
            case ']':
            case '+':
            case '-':
            case '_':
                validName.Add(c);
                break;
            default:
                if (StringUtils::IsAlnum(c))
                    validName.Add(c);
            }
        }
        validName.Add('\0');

        sb.Append(TEXT("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"));
        sb.Append(TEXT("<Game configVersion=\"0\">\n"));
        sb.AppendFormat(TEXT("  <Identity Name=\"{0}\" Publisher=\"{1}\" Version=\"{2}\"/>\n"),
                        validName.Get(),
                        platformSettings->PublisherName.HasChars() ? platformSettings->PublisherName : TEXT("CN=") + gameSettings->CompanyName,
                        project->Version.ToString(4));
        sb.Append(TEXT("  <ExecutableList>\n"));
        sb.AppendFormat(TEXT("    <Executable Name=\"{0}\"\n"), executableFilename);
        switch (data.Platform)
        {
        case BuildPlatform::XboxOne:
            sb.Append(TEXT("                TargetDeviceFamily=\"XboxOne\"\n"));
            break;
        case BuildPlatform::XboxScarlett:
            sb.Append(TEXT("                TargetDeviceFamily=\"Scarlett\"\n"));
            break;
        default:
            sb.Append(TEXT("                TargetDeviceFamily=\"PC\"\n"));
            break;
        }
        sb.Append(TEXT("                IsDevOnly=\"false\"\n"));
        sb.Append(TEXT("                Id=\"Game\"\n"));
        sb.Append(TEXT("    />"));
        sb.Append(TEXT("  </ExecutableList>\n"));
        sb.AppendFormat(TEXT("  <ShellVisuals DefaultDisplayName=\"{0}\"\n"), gameSettings->ProductName);
        sb.AppendFormat(TEXT("                PublisherDisplayName=\"{0}\"\n"), platformSettings->PublisherDisplayName.HasChars() ? platformSettings->PublisherDisplayName : gameSettings->CompanyName);
        sb.AppendFormat(TEXT("                BackgroundColor=\"#{0}\"\n"), platformSettings->BackgroundColor.ToHexString().Left(6));
        sb.AppendFormat(TEXT("                ForegroundText=\"{0}\"\n"), platformSettings->ForegroundText);
        sb.Append(TEXT("                Square150x150Logo=\"Assets\\Square150x150Logo.png\"\n"));
        sb.Append(TEXT("                Square480x480Logo=\"Assets\\Square480x480Logo.png\"\n"));
        sb.Append(TEXT("                Square44x44Logo=\"Assets\\Square44x44Logo.png\"\n"));
        sb.Append(TEXT("                StoreLogo=\"Assets\\StoreLogo.png\"\n"));
        sb.Append(TEXT("                SplashScreenImage=\"Assets\\SplashScreenImage.png\"\n"));
        sb.Append(TEXT("    />\n"));
        if (platformSettings->TitleId.HasChars())
            sb.AppendFormat(TEXT("  <TitleId>{0}</TitleId>\n"), platformSettings->TitleId);
        if (platformSettings->StoreId.HasChars())
            sb.AppendFormat(TEXT("  <StoreId>{0}</StoreId>\n"), platformSettings->StoreId);
        sb.AppendFormat(TEXT("  <RequiresXboxLive>{0}</RequiresXboxLive>\n"), platformSettings->RequiresXboxLive);
        sb.Append(TEXT("  <MediaCapture>\n"));
        sb.AppendFormat(TEXT("    <GameDVRSystemComponent>{0}</GameDVRSystemComponent>\n"), platformSettings->GameDVRSystemComponent);
        sb.AppendFormat(TEXT("    <BlockBroadcast>{0}</BlockBroadcast>\n"), platformSettings->BlockBroadcast);
        sb.AppendFormat(TEXT("    <BlockGameDVR>{0}</BlockGameDVR>\n"), platformSettings->BlockGameDVR);
        sb.Append(TEXT("  </MediaCapture>\n"));
        sb.Append(TEXT("</Game>\n"));

        if (File::WriteAllText(configFilePath, sb, Encoding::ANSI))
        {
            LOG(Error, "Failed to create config file.");
            return true;
        }
    }

    // Remove previous package file
    const String packageOutputPath = data.DataOutputPath / TEXT("Package");
    if (FileSystem::DirectoryExists(packageOutputPath))
        FileSystem::DeleteDirectory(packageOutputPath);

    // Generate package layout
    data.StepProgress(TEXT("Generating package layout"), 0.3f);
    const String gdkBinPath = _gdkPath / TEXT("../bin");
    const String makePkgPath = gdkBinPath / TEXT("MakePkg.exe");
    CreateProcessSettings procSettings;
    procSettings.FileName = String::Format(TEXT("\"{0}\" genmap /f layout.xml /d \"{1}\""), makePkgPath, data.DataOutputPath);
    procSettings.WorkingDirectory = data.DataOutputPath;
    const int32 result = Platform::CreateProcess(procSettings);
    if (result != 0)
    {
        data.Error(TEXT("Failed to generate package layout."));
        return true;
    }

    return false;
}

#endif
