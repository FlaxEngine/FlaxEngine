// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_UWP || PLATFORM_TOOLS_XBOX_ONE

#include "UWPPlatformTools.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/UWP/UWPPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"

IMPLEMENT_SETTINGS_GETTER(UWPPlatformSettings, UWPPlatform);

bool UWPPlatformTools::UseAOT() const
{
    return true;
}

bool UWPPlatformTools::OnScriptsStepDone(CookingData& data)
{
    // Override Newtonsoft.Json.dll for some platforms (that don't support runtime code generation)
    const String customBinPath = data.GetPlatformBinariesRoot() / TEXT("Newtonsoft.Json.dll");
    const String assembliesPath = data.OutputPath;
    if (FileSystem::CopyFile(assembliesPath / TEXT("Newtonsoft.Json.dll"), customBinPath))
    {
        data.Error(TEXT("Failed to copy deploy custom assembly."));
        return true;
    }
    FileSystem::DeleteFile(assembliesPath / TEXT("Newtonsoft.Json.pdb"));

    return false;
}

bool UWPPlatformTools::OnDeployBinaries(CookingData& data)
{
    bool isXboxOne = data.Platform == BuildPlatform::XboxOne;
    const auto platformDataPath = Globals::StartupFolder / TEXT("Source/Platforms");
    const auto uwpDataPath = platformDataPath / (isXboxOne ? TEXT("XboxOne") : TEXT("UWP")) / TEXT("Binaries");
    const auto gameSettings = GameSettings::Get();
    const auto platformSettings = UWPPlatformSettings::Get();
    Array<byte> fileTemplate;

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
            data.Error(TEXT("Missing source file {0}."), files[i]);
            return true;
        }

        if (FileSystem::CopyFile(data.OutputPath / StringUtils::GetFileName(files[i]), files[i]))
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
    case BuildPlatform::XboxOne:
        mode = "x64";
        break;
    default:
        return true;
    }

    // Prepare certificate
    const auto srcCertificatePath = Globals::ProjectFolder / platformSettings->CertificateLocation;
    const auto dstCertificatePath = data.OutputPath / TEXT("WSACertificate.pfx");
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
    const auto dstAssetsPath = data.OutputPath / TEXT("Assets");
    const auto srcAssetsPath = uwpDataPath / TEXT("Assets");
    if (!FileSystem::DirectoryExists(dstAssetsPath))
    {
        if (FileSystem::CopyDirectory(dstAssetsPath, srcAssetsPath, true))
        {
            data.Error(TEXT("Failed to copy Assets directory."));
            return true;
        }
    }
    const auto dstPropertiesPath = data.OutputPath / TEXT("Properties");
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
        if (File::ReadAllBytes(srcAssemblyInfoPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load AssemblyInfo.cs template."));
            return true;
        }
        fileTemplate[fileTemplate.Count() - 1] = 0;

        // Write data to file
        auto file = FileWriteStream::Open(dstAssemblyInfoPath);
        bool hasError = true;
        if (file)
        {
            auto now = DateTime::Now();
            file->WriteTextFormatted(
                (char*)fileTemplate.Get()
                , gameSettings->ProductName.ToStringAnsi()
                , gameSettings->CompanyName.ToStringAnsi()
                , now.GetYear()
            );
            hasError = file->HasError();
            Delete(file);
        }
        if (hasError)
        {
            data.Error(TEXT("Failed to create AssemblyInfo.cs."));
            return true;
        }
    }
    const auto dstAppPath = data.OutputPath / TEXT("App.cs");
    const auto srcAppPath = uwpDataPath / TEXT("App.cs");
    if (!FileSystem::FileExists(dstAppPath))
    {
        // Get template
        if (File::ReadAllBytes(srcAppPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load App.cs template."));
            return true;
        }
        fileTemplate[fileTemplate.Count() - 1] = 0;

        // Write data to file
        auto file = FileWriteStream::Open(dstAppPath);
        bool hasError = true;
        if (file)
        {
            file->WriteTextFormatted(
                (char*)fileTemplate.Get()
                , defaultNamespace.ToStringAnsi() // {0} Default Namespace
            );
            hasError = file->HasError();
            Delete(file);
        }
        if (hasError)
        {
            data.Error(TEXT("Failed to create App.cs."));
            return true;
        }
    }
    const auto dstFlaxGeneratedPath = data.OutputPath / TEXT("FlaxGenerated.cs");
    const auto srcFlaxGeneratedPath = uwpDataPath / TEXT("FlaxGenerated.cs");
    {
        // Get template
        if (File::ReadAllBytes(srcFlaxGeneratedPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load FlaxGenerated.cs template."));
            return true;
        }
        fileTemplate[fileTemplate.Count() - 1] = 0;

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
        if (isXboxOne)
            preferredLaunchWindowingMode = "FullScreen";

        // Write data to file
        auto file = FileWriteStream::Open(dstFlaxGeneratedPath);
        bool hasError = true;
        if (file)
        {
            file->WriteTextFormatted(
                (char*)fileTemplate.Get()
                , autoRotationPreferences.Get()
                , preferredLaunchWindowingMode.Get()
            );
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
    const auto dstSolutionPath = data.OutputPath / projectName + TEXT(".sln");
    const auto srcSolutionPath = uwpDataPath / TEXT("Solution.sln");
    if (!FileSystem::FileExists(dstSolutionPath))
    {
        // Get template
        if (File::ReadAllBytes(srcSolutionPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load Solution.sln template."));
            return true;
        }
        fileTemplate[fileTemplate.Count() - 1] = 0;

        // Write data to file
        auto file = FileWriteStream::Open(dstSolutionPath);
        bool hasError = true;
        if (file)
        {
            file->WriteTextFormatted(
                (char*)fileTemplate.Get()
                , projectName.ToStringAnsi() // {0} Project Name
                , mode // {1} Platform Mode
                , projectGuid.ToStringAnsi() // {2} Project ID
            );
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
    const auto dstProjectPath = data.OutputPath / projectName + TEXT(".csproj");
    const auto srcProjectPath = uwpDataPath / TEXT("Project.csproj");
    {
        // Get template
        if (File::ReadAllBytes(srcProjectPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load Project.csproj template."));
            return true;
        }
        fileTemplate[fileTemplate.Count() - 1] = 0;

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
            file->WriteTextFormatted(
                (char*)fileTemplate.Get()
                , projectName.ToStringAnsi() // {0} Project Name
                , mode // {1} Platform Mode
                , projectGuid.Get() // {2} Project ID
                , filesInclude.ToString().ToStringAnsi() // {3} Files to include
                , defaultNamespace.ToStringAnsi() // {4} Default Namespace
            );
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
    const auto dstManifestPath = data.OutputPath / TEXT("Package.appxmanifest");
    const auto srcManifestPath = uwpDataPath / TEXT("Package.appxmanifest");
    if (!FileSystem::FileExists(dstManifestPath))
    {
        // Get template
        if (File::ReadAllBytes(srcManifestPath, fileTemplate))
        {
            data.Error(TEXT("Failed to load Package.appxmanifest template."));
            return true;
        }
        fileTemplate[fileTemplate.Count() - 1] = 0;

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
            file->WriteTextFormatted(
                (char*)fileTemplate.Get()
                , projectName.ToStringAnsi() // {0} Display Name
                , gameSettings->CompanyName.ToStringAnsi() // {1} Company Name
                , productId.ToStringAnsi() // {2} Product ID
                , defaultNamespace.ToStringAnsi() // {3} Default Namespace
            );
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

void UWPPlatformTools::OnConfigureAOT(CookingData& data, AotConfig& config)
{
    const auto platformDataPath = data.GetPlatformBinariesRoot();
    const bool useInterpreter = true; // TODO: support using Full AOT instead of interpreter
    const bool enableDebug = data.Configuration != BuildConfiguration::Release;
    const Char* aotMode = useInterpreter ? TEXT("full,interp") : TEXT("full");
    const Char* debugMode = enableDebug ? TEXT("soft-debug") : TEXT("nodebug");
    config.AotCompilerArgs = String::Format(TEXT("--aot={0},verbose,stats,print-skipped,{1} -O=all"),
                                            aotMode,
                                            debugMode);
    if (enableDebug)
        config.AotCompilerArgs = TEXT("--debug ") + config.AotCompilerArgs;
    config.AotCompilerPath = platformDataPath / TEXT("Tools/mono.exe");
}

bool UWPPlatformTools::OnPerformAOT(CookingData& data, AotConfig& config, const String& assemblyPath)
{
    // Skip .dll.dll which could be a false result from the previous AOT which could fail
    if (assemblyPath.EndsWith(TEXT(".dll.dll")))
    {
        LOG(Warning, "Skip AOT for file '{0}' as it can be a result from the previous task", assemblyPath);
        return false;
    }

    // Check if skip this assembly (could be already processed)
    const String filename = StringUtils::GetFileName(assemblyPath);
    const String outputPath = config.AotCachePath / filename + TEXT(".dll");
    if (FileSystem::FileExists(outputPath) && FileSystem::GetFileLastEditTime(assemblyPath) < FileSystem::GetFileLastEditTime(outputPath))
        return false;
    LOG(Info, "Calling AOT tool for \"{0}\"", assemblyPath);

    // Cleanup temporary results (fromm the previous AT that fail or sth)
    const String resultPath = assemblyPath + TEXT(".dll");
    const String resultPathExp = resultPath + TEXT(".exp");
    const String resultPathLib = resultPath + TEXT(".lib");
    const String resultPathPdb = resultPath + TEXT(".pdb");
    if (FileSystem::FileExists(resultPath))
        FileSystem::DeleteFile(resultPath);
    if (FileSystem::FileExists(resultPathExp))
        FileSystem::DeleteFile(resultPathExp);
    if (FileSystem::FileExists(resultPathLib))
        FileSystem::DeleteFile(resultPathLib);
    if (FileSystem::FileExists(resultPathPdb))
        FileSystem::DeleteFile(resultPathPdb);

    // Call tool
    String workingDir = StringUtils::GetDirectoryName(config.AotCompilerPath);
    String command = String::Format(TEXT("\"{0}\" {1} \"{2}\""), config.AotCompilerPath, config.AotCompilerArgs, assemblyPath);
    const int32 result = Platform::RunProcess(command, workingDir, config.EnvVars);
    if (result != 0)
    {
        data.Error(TEXT("AOT tool execution failed with result code {1} for assembly \"{0}\". See log for more info."), assemblyPath, result);
        return true;
    }

    // Copy result
    if (FileSystem::CopyFile(outputPath, resultPath))
    {
        data.Error(TEXT("Failed to copy the AOT tool result file. It can be missing."));
        return true;
    }

    // Copy pdb file if exists
    if (data.Configuration != BuildConfiguration::Release && FileSystem::FileExists(resultPathPdb))
    {
        FileSystem::CopyFile(config.AotCachePath / StringUtils::GetFileName(resultPathPdb), resultPathPdb);
    }

    // Clean intermediate results
    if (FileSystem::DeleteFile(resultPath)
        || (FileSystem::FileExists(resultPathExp) && FileSystem::DeleteFile(resultPathExp))
        || (FileSystem::FileExists(resultPathLib) && FileSystem::DeleteFile(resultPathLib))
        || (FileSystem::FileExists(resultPathPdb) && FileSystem::DeleteFile(resultPathPdb))
    )
    {
        LOG(Warning, "Failed to remove the AOT tool result file(s).");
    }

    return false;
}

bool UWPPlatformTools::OnPostProcess(CookingData& data)
{
    // Special case for UWP
    // FlaxEngine.dll cannot be added to the solution as `Content` item (due to conflicts with C++ /CX FlaxEngine.dll)
    // Use special directory for it (generated UWP project handles this case and copies lib to the output)
    const String assembliesPath = data.OutputPath;
    const auto dstPath1 = data.OutputPath / TEXT("DataSecondary");
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

const Char* WSAPlatformTools::GetDisplayName() const
{
    return TEXT("Windows Store");
}

const Char* WSAPlatformTools::GetName() const
{
    return TEXT("UWP");
}

PlatformType WSAPlatformTools::GetPlatform() const
{
    return PlatformType::UWP;
}

ArchitectureType WSAPlatformTools::GetArchitecture() const
{
    return _arch;
}

const Char* XboxOnePlatformTools::GetDisplayName() const
{
    return TEXT("Xbox One");
}

const Char* XboxOnePlatformTools::GetName() const
{
    return TEXT("XboxOne");
}

PlatformType XboxOnePlatformTools::GetPlatform() const
{
    return PlatformType::XboxOne;
}

ArchitectureType XboxOnePlatformTools::GetArchitecture() const
{
    return ArchitectureType::x64;
}

#endif
