// Copyright (c) Wojciech Figat. All rights reserved.

#include "CompileScriptsStep.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Serialization/Json.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Editor/Cooker/PlatformTools.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Engine/Engine/Globals.h"
#if PLATFORM_MAC
#include <sys/stat.h>
#endif

bool CompileScriptsStep::DeployBinaries(CookingData& data, const String& path, const String& projectFolderPath)
{
    if (_deployedBuilds.Contains(path))
        return false;
    LOG(Info, "Deploying binaries from build {0}", path);
    _deployedBuilds.Add(path);

    // Read file contents
    Array<byte> fileData;
    if (File::ReadAllBytes(path, fileData))
    {
        LOG(Error, "Failed to read file {0} contents.", path);
        return true;
    }

    // Parse Json data
    rapidjson_flax::Document document;
    document.Parse((const char*)fileData.Get(), fileData.Count());
    if (document.HasParseError())
    {
        LOG(Error, "Failed to parse {0} file contents.", path);
        return true;
    }

    // Deploy all references
    auto referencesMember = document.FindMember("References");
    if (referencesMember != document.MemberEnd())
    {
        auto& referencesArray = referencesMember->value;
        ASSERT(referencesArray.IsArray());
        for (rapidjson::SizeType i = 0; i < referencesArray.Size(); i++)
        {
            auto& reference = referencesArray[i];
            String referenceProjectPath = JsonTools::GetString(reference, "ProjectPath", String::Empty);
            String referencePath = JsonTools::GetString(reference, "Path", String::Empty);
            if (referenceProjectPath.IsEmpty() || referencePath.IsEmpty())
            {
                LOG(Error, "Empty reference in {0}.", path);
                return true;
            }

            Scripting::ProcessBuildInfoPath(referenceProjectPath, projectFolderPath);
            Scripting::ProcessBuildInfoPath(referencePath, projectFolderPath);

            String referenceProjectFolderPath = StringUtils::GetDirectoryName(referenceProjectPath);

            if (DeployBinaries(data, referencePath, referenceProjectFolderPath))
            {
                LOG(Error, "Failed to load reference in {0} to {1}.", path, referenceProjectPath);
                return true;
            }
        }
    }

    // Deploy all binary modules
    auto binaryModulesMember = document.FindMember("BinaryModules");
    if (binaryModulesMember != document.MemberEnd())
    {
        auto& binaryModulesArray = binaryModulesMember->value;
        ASSERT(binaryModulesArray.IsArray());
        for (rapidjson::SizeType i = 0; i < binaryModulesArray.Size(); i++)
        {
            auto& binaryModule = binaryModulesArray[i];
            auto& e = data.BinaryModules.AddOne();
            const auto nameMember = binaryModule.FindMember("Name");
            if (nameMember == binaryModule.MemberEnd())
            {
                LOG(Error, "Failed to process file {0}. Missing binary module name.", path);
                return true;
            }
            e.Name = nameMember->value.GetText();
            StringAnsi nameAnsi(nameMember->value.GetString(), nameMember->value.GetStringLength());
            e.NativePath = JsonTools::GetString(binaryModule, "NativePath", String::Empty);
            e.ManagedPath = JsonTools::GetString(binaryModule, "ManagedPath", String::Empty);

            Scripting::ProcessBuildInfoPath(e.NativePath, projectFolderPath);
            Scripting::ProcessBuildInfoPath(e.ManagedPath, projectFolderPath);

            e.NativePath = String(StringUtils::GetFileName(e.NativePath));
            e.ManagedPath = String(StringUtils::GetFileName(e.ManagedPath));

            LOG(Info, "Collecting binary module {0}", e.Name);
        }
    }

    // Deploy files
    Array<String> files(16);
    const String outputPath = StringUtils::GetDirectoryName(path);
    FileSystem::DirectoryGetFiles(files, outputPath, TEXT("*"), DirectorySearchOption::TopDirectoryOnly);
    for (int32 i = files.Count() - 1; i >= 0; i--)
    {
        bool skip = false;
        const String& file = files[i];
        for (auto& extension : _extensionsToSkip)
        {
            if (file.EndsWith(extension))
            {
                skip = true;
                break;
            }
        }
        if (skip)
            files.RemoveAt(i);
    }
    for (auto& file : files)
    {
        const String& dstPath = data.Tools->IsNativeCodeFile(data, file) ? data.NativeCodeOutputPath : data.ManagedCodeOutputPath;
        const String dst = dstPath / StringUtils::GetFileName(file);
        if (dst == file)
            continue;
        if (FileSystem::CopyFile(dst, file))
        {
            data.Error(String::Format(TEXT("Failed to copy file from {0} to {1}."), file, dst));
            return true;
        }

#if PLATFORM_MAC
        // Ensure to keep valid file permissions for executable files
        const StringAsANSI<> fileANSI(*file, file.Length());
        const StringAsANSI<> dstANSI(*dst, dst.Length());
        struct stat st;
        stat(fileANSI.Get(), &st);
        chmod(dstANSI.Get(), st.st_mode);
#endif
    }

    return false;
}

bool CompileScriptsStep::Perform(CookingData& data)
{
    data.StepProgress(TEXT("Compiling game scripts"), 0);

    const ProjectInfo* project = Editor::Project;
    String target = project->GameTarget;
    StringView workingDir;
    const Char *platform, *architecture, *configuration = ::ToString(data.Configuration);
    data.GetBuildPlatformName(platform, architecture);
    String targetBuildInfo = project->ProjectFolderPath / TEXT("Binaries") / target / platform / architecture / configuration / target + TEXT(".Build.json");
    if (target.IsEmpty())
    {
        // Fallback to engine-only if game has no code
        LOG(Warning, "Empty GameTarget in project.");
        target = TEXT("FlaxGame");
        workingDir = Globals::StartupFolder;
        targetBuildInfo = Globals::StartupFolder / TEXT("Source/Platforms") / platform / TEXT("Binaries") / TEXT("Game") / architecture / configuration / target + TEXT(".Build.json");
    }
    _extensionsToSkip.Clear();
    _extensionsToSkip.Add(TEXT(".exp"));
    _extensionsToSkip.Add(TEXT(".ilk"));
    _extensionsToSkip.Add(TEXT(".lib"));
    _extensionsToSkip.Add(TEXT(".a"));
    _extensionsToSkip.Add(TEXT(".Build.json"));
    _extensionsToSkip.Add(TEXT(".DS_Store"));
    if (data.Configuration == BuildConfiguration::Release)
    {
        _extensionsToSkip.Add(TEXT(".xml"));
        _extensionsToSkip.Add(TEXT(".pdb"));
    }
    _deployedBuilds.Clear();
    data.BinaryModules.Clear();

    if (data.Tools->OnScriptsCompilationStart(data))
        return true;

    BUILD_STEP_CANCEL_CHECK;

    // Compile the scripts
    LOG(Info, "Starting scripts compilation for game...");
    const String logFile = data.CacheDirectory / TEXT("CompileLog.txt");
    auto args = String::Format(
        TEXT("-log -logfile=\"{4}\" -build -mutex -buildtargets={0} -platform={1} -arch={2} -configuration={3} -aotMode={5} {6}"),
        target, platform, architecture, configuration, logFile, ToString(data.Tools->UseAOT()), GAME_BUILD_DOTNET_VER);
#if PLATFORM_WINDOWS
    if (data.Platform == BuildPlatform::LinuxX64)
#elif PLATFORM_LINUX
    if (data.Platform == BuildPlatform::Windows64 || data.Platform == BuildPlatform::Windows32)
#else
    if (false)
#endif
    {
        // Skip building C++ (no need to install cross-toolchain to build C#-only game)
        args += TEXT(" -BuildBindingsOnly");

        // Assume FlaxGame was prebuilt for target platform
        args += TEXT(" -SkipTargets=FlaxGame");
    }
    for (const String& define : data.CustomDefines)
    {
        args += TEXT(" -D");
        args += define;
    }
    if (ScriptsBuilder::RunBuildTool(args, workingDir))
    {
        data.Error(TEXT("Failed to compile game scripts."));
        return true;
    }

    BUILD_STEP_CANCEL_CHECK;

    if (data.Tools->OnScriptsCompilationEnd(data))
        return true;

    data.StepProgress(TEXT("Exporting binaries"), 0.8f);

    // Deploy binary modules
    if (DeployBinaries(data, targetBuildInfo, project->ProjectFolderPath))
        return true;

    data.StepProgress(TEXT("Generating merged build info"), 0.95f);

    // Generate merged build info for all deployed binary modules
    {
        rapidjson_flax::StringBuffer buffer;
#if BUILD_DEBUG
        PrettyJsonWriter writerObj(buffer);
#else
        CompactJsonWriter writerObj(buffer);
#endif
        JsonWriter& writer = writerObj;

        writer.StartObject();
        {
            writer.JKEY("Name");
            writer.String(target);
            writer.JKEY("Platform");
            writer.String(platform);
            writer.JKEY("Configuration");
            writer.String(configuration);

            writer.JKEY("BinaryModules");
            writer.StartArray();
            for (auto& binaryModule : data.BinaryModules)
            {
                writer.StartObject();

                writer.JKEY("Name");
                writer.String(binaryModule.Name);

                if (binaryModule.NativePath.HasChars())
                {
                    writer.JKEY("NativePath");
                    writer.String(binaryModule.NativePath);
                }

                if (binaryModule.ManagedPath.HasChars())
                {
                    writer.JKEY("ManagedPath");
                    writer.String(binaryModule.ManagedPath);
                }

                writer.EndObject();
            }
            writer.EndArray();
        }
        writer.EndObject();

        const String outputBuildInfo = data.DataOutputPath / TEXT("Game.Build.json");
        if (File::WriteAllBytes(outputBuildInfo, (byte*)buffer.GetString(), (int32)buffer.GetSize()))
        {
            LOG(Error, "Failed to save binary modules info file {0}.", outputBuildInfo);
            return true;
        }
    }

    if (data.Tools->OnScriptsStepDone(data))
        return true;

    return false;
}
