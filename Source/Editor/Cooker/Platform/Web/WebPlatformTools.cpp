// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_WEB

#include "WebPlatformTools.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/Web/WebPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Editor/Cooker/GameCooker.h"

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
    return DotNetAOTModes::NoDotnet;
}

PixelFormat WebPlatformTools::GetTextureFormat(CookingData& data, TextureBase* texture, PixelFormat format)
{
    // TODO: texture compression for Web (eg. ASTC for mobile and BC for others?)
    return PixelFormatExtensions::FindUncompressedFormat(format);
}

bool WebPlatformTools::IsNativeCodeFile(CookingData& data, const String& file)
{
    String extension = FileSystem::GetExtension(file);
    return extension.IsEmpty() || extension == TEXT("html") || extension == TEXT("js") || extension == TEXT("wasm");
}

void WebPlatformTools::OnBuildStarted(CookingData& data)
{
    // Adjust the cooking output folder for the data files so file_packager tool can compress and output final data inside the cooker output folder
    data.DataOutputPath = data.CacheDirectory / TEXT("Files");
}

bool WebPlatformTools::OnPostProcess(CookingData& data)
{
    const auto gameSettings = GameSettings::Get();
    const auto platformSettings = WebPlatformSettings::Get();
    const auto platformDataPath = data.GetPlatformBinariesRoot();

    // Get name of the output binary (JavaScript and WebAssembly files match)
    String gameJs;
    {
        Array<String> files;
        FileSystem::DirectoryGetFiles(files, data.OriginalOutputPath, TEXT("*"), DirectorySearchOption::TopDirectoryOnly);
        for (const String& file : files)
        {
            if (file.EndsWith(TEXT(".js")))
            {
                String outputWasm = String(StringUtils::GetPathWithoutExtension(file)) + TEXT(".wasm");
                if (files.Contains(outputWasm))
                {
                    gameJs = file;
                    break;
                }
            }
        }
    }
    if (gameJs.IsEmpty())
    {
        data.Error(TEXT("Failed to find the main JavaScript for the output game"));
        return true;
    }

    // Move .wasm assemblies into the data files in order for dlopen to work (blocking)
    {
        Array<String> files;
        FileSystem::DirectoryGetFiles(files, data.OriginalOutputPath, TEXT("*.wasm"), DirectorySearchOption::AllDirectories);
        StringView gameWasm = StringUtils::GetFileNameWithoutExtension(gameJs);
        for (const String& file : files)
        {
            if (StringUtils::GetFileNameWithoutExtension(file) == gameWasm)
                continue; // Skip the main game module
            FileSystem::MoveFile(data.DataOutputPath / StringUtils::GetFileName(file), file, true);
        }
    }

    // Pack data files into a single file using Emscripten's file_packager tool
    {
        CreateProcessSettings procSettings;
        String emscriptenSdk = TEXT("EMSDK");
        Platform::GetEnvironmentVariable(emscriptenSdk, emscriptenSdk);
        procSettings.FileName = emscriptenSdk / TEXT("upstream/emscripten/tools/file_packager");
#if PLATFORM_WIN32
        procSettings.FileName += TEXT(".bat");
#endif
        procSettings.Arguments = String::Format(TEXT("files.data --preload \"{}@/\" --lz4 --js-output=files.js"), data.DataOutputPath);
        procSettings.WorkingDirectory = data.OriginalOutputPath;
        const int32 result = Platform::CreateProcess(procSettings);
        if (result != 0)
        {
            if (!FileSystem::FileExists(procSettings.FileName))
                data.Error(TEXT("Missing file_packager.bat. Ensure Emscripten SDK installation is valid and 'EMSDK' environment variable points to it."));
            data.Error(String::Format(TEXT("Failed to package project files (result code: {0}). See log for more info."), result));
            return true;
        }
    }

    // TODO: customizable HTML templates

    // Insert packaged file system with game data
    {
        String gameJsText;
        if (File::ReadAllText(gameJs, gameJsText))
        {
            data.Error(String::Format(TEXT("Failed to load file '{}'"), gameJs));
            return true;
        }
        const String filesIncludeBegin = TEXT("// include: files.js");
        const String filesIncludeEnd = TEXT("// end include: files.js");
        String fileJs = data.OriginalOutputPath / TEXT("files.js");
        if (!gameJsText.Contains(filesIncludeBegin))
        {
            // Insert generated files.js into the main game file after the minimum_runtime_check.js include
            String fileJsText;
            if (File::ReadAllText(fileJs, fileJsText))
            {
                data.Error(String::Format(TEXT("Failed to load file '{}'"), fileJs));
                return true;
            }
            const String insertPrefixLocation = TEXT("// end include: minimum_runtime_check.js");
            int32 location = gameJsText.Find(insertPrefixLocation);
            CHECK_RETURN(location != -1, true);
            location += insertPrefixLocation.Length() + 1;
            fileJsText = filesIncludeBegin + TEXT("\n") + fileJsText + TEXT("\n") + filesIncludeEnd + TEXT("\n");
            gameJsText.Insert(location, fileJsText);
            File::WriteAllText(gameJs, gameJsText, Encoding::UTF8);
        }

        // Remove the generated files.js as it's now included in the main game JS file
        FileSystem::DeleteFile(fileJs);
    }

    const auto buildSettings = BuildSettings::Get();
    if (buildSettings->SkipPackaging)
        return false;
    GameCooker::PackageFiles();

    // TODO: minify/compress output JS files (in Release builds)

    LOG(Info, "Output website size: {0} MB", FileSystem::GetDirectorySize(data.OriginalOutputPath) / 1024 / 1024);

    return false;
}

#endif
