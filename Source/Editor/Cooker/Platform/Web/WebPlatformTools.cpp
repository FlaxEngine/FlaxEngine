// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_WEB

#include "WebPlatformTools.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/Web/WebPlatformSettings.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Textures/TextureBase.h"
#include "Editor/Cooker/GameCooker.h"

IMPLEMENT_SETTINGS_GETTER(WebPlatformSettings, WebPlatform);

namespace
{
    struct WebPlatformCache
    {
        WebPlatformSettings::TextureCompression TexturesCompression;
    };
}

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
    const auto platformSettings = WebPlatformSettings::Get();
    const auto uncompressed = PixelFormatExtensions::FindUncompressedFormat(format);
    switch (platformSettings->TexturesCompression)
    {
    case WebPlatformSettings::TextureCompression::Uncompressed:
        return uncompressed;
    case WebPlatformSettings::TextureCompression::BC:
        return format;
    case WebPlatformSettings::TextureCompression::ASTC:
        switch (format)
        {
        case PixelFormat::BC4_SNorm:
            return PixelFormat::R8_SNorm;
        case PixelFormat::BC5_SNorm:
            return PixelFormat::R16G16_SNorm;
        case PixelFormat::BC6H_Typeless:
        case PixelFormat::BC6H_Uf16:
        case PixelFormat::BC6H_Sf16:
        case PixelFormat::BC7_Typeless:
        case PixelFormat::BC7_UNorm:
        case PixelFormat::BC7_UNorm_sRGB:
            return PixelFormat::R16G16B16A16_Float; // TODO: ASTC HDR
        default:
            return PixelFormatExtensions::IsSRGB(format) ? PixelFormat::ASTC_6x6_UNorm_sRGB : PixelFormat::ASTC_6x6_UNorm;
        }
    case WebPlatformSettings::TextureCompression::Basis:
        switch (format)
        {
        case PixelFormat::BC7_Typeless:
        case PixelFormat::BC7_UNorm:
        case PixelFormat::BC7_UNorm_sRGB:
            return PixelFormat::R16G16B16A16_Float; // Basic Universal doesn't support alpha in BC7 (and it can be loaded only from LDR formats)
        default:
            if (uncompressed != format && texture->Size().MinValue() >= 16)
                return PixelFormat::Basis;
            return uncompressed;
        }
    default:
        return format;
    }
}

void WebPlatformTools::LoadCache(CookingData& data, IBuildCache* cache, const Span<byte>& bytes)
{
    const auto platformSettings = WebPlatformSettings::Get();
    bool invalidTextures = true;
    if (bytes.Length() == sizeof(WebPlatformCache))
    {
        auto* platformCache = (WebPlatformCache*)bytes.Get();
        invalidTextures = platformCache->TexturesCompression != platformSettings->TexturesCompression;
    }
    if (invalidTextures)
    {
        LOG(Info, "{0} option has been modified.", TEXT("TexturesQuality"));
        cache->InvalidateCacheTextures();
    }
}

Array<byte> WebPlatformTools::SaveCache(CookingData& data, IBuildCache* cache)
{
    const auto platformSettings = WebPlatformSettings::Get();
    WebPlatformCache platformCache;
    platformCache.TexturesCompression = platformSettings->TexturesCompression;
    Array<byte> result;
    result.Add((const byte*)&platformCache, sizeof(platformCache));
    return result;
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

    // Copy icon file
    {
        String dstIcon = data.OriginalOutputPath / TEXT("favicon.ico");
        if (!FileSystem::FileExists(dstIcon))
            FileSystem::CopyFile(dstIcon, platformDataPath / TEXT("favicon.ico"));
    }

    // Copy custom HTMl template
    auto customHtml = platformSettings->CustomHtml.TrimTrailing();
    if (customHtml.HasChars())
    {
        FileSystem::CopyFile(data.OriginalOutputPath / TEXT("FlaxGame.html"), customHtml);
    }

    // Rename game website main HTML file to match the most common name used by web servers (index.html)
    FileSystem::MoveFile(data.OriginalOutputPath / TEXT("index.html"), data.OriginalOutputPath / TEXT("FlaxGame.html"), true);

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
            if (location != -1)
            {
                location += insertPrefixLocation.Length() + 1;
                fileJsText = filesIncludeBegin + TEXT("\n") + fileJsText + TEXT("\n") + filesIncludeEnd + TEXT("\n");
                gameJsText.Insert(location, fileJsText);
            }
            else
            {
                // Comments are missing in Release when JS/HTML are minified
                fileJsText.Insert(0, filesIncludeBegin);
                fileJsText.Insert(0, TEXT("\n"));
                gameJsText.Insert(0, fileJsText);
            }
            File::WriteAllText(gameJs, gameJsText, Encoding::UTF8);
        }

        // Remove the generated files.js as it's now included in the main game JS file
        FileSystem::DeleteFile(fileJs);
    }

    const auto buildSettings = BuildSettings::Get();
    if (buildSettings->SkipPackaging)
        return false;
    GameCooker::PackageFiles();

    LOG(Info, "Output website size: {0} MB", FileSystem::GetDirectorySize(data.OriginalOutputPath) / 1024 / 1024);

    return false;
}

#endif
