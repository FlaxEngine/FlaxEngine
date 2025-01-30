// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "EditorUtilities.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Utilities/StringConverter.h"
#if PLATFORM_MAC
#include "Engine/Platform/Apple/ApplePlatformSettings.h"
#endif

String EditorUtilities::GetOutputName()
{
    const auto gameSettings = GameSettings::Get();
    const auto buildSettings = BuildSettings::Get();
    String outputName = buildSettings->OutputName;
    outputName.Replace(TEXT("${PROJECT_NAME}"), *gameSettings->ProductName, StringSearchCase::IgnoreCase);
    outputName.Replace(TEXT("${COMPANY_NAME}"), *gameSettings->CompanyName, StringSearchCase::IgnoreCase);
    if (outputName.IsEmpty())
        outputName = TEXT("FlaxGame");
    ValidatePathChars(outputName, 0);
    return outputName;
}

bool EditorUtilities::FormatAppPackageName(String& packageName)
{
    const auto gameSettings = GameSettings::Get();
    String productName = gameSettings->ProductName;
    productName.Replace(TEXT(" "), TEXT(""));
    productName.Replace(TEXT("."), TEXT(""));
    productName.Replace(TEXT("-"), TEXT(""));
    String companyName = gameSettings->CompanyName;
    companyName.Replace(TEXT(" "), TEXT(""));
    companyName.Replace(TEXT("."), TEXT(""));
    companyName.Replace(TEXT("-"), TEXT(""));
    packageName.Replace(TEXT("${PROJECT_NAME}"), *productName, StringSearchCase::IgnoreCase);
    packageName.Replace(TEXT("${COMPANY_NAME}"), *companyName, StringSearchCase::IgnoreCase);
    packageName = packageName.ToLower();
    for (int32 i = 0; i < packageName.Length(); i++)
    {
        const auto c = packageName[i];
        if (c != '_' && c != '.' && !StringUtils::IsAlnum(c))
        {
            LOG(Error, "App identifier \'{0}\' contains invalid character. Only letters, numbers, dots and underscore characters are allowed.", packageName);
            return true;
        }
    }
    if (packageName.IsEmpty())
    {
        LOG(Error, "App identifier is empty.", packageName);
        return true;
    }
    return false;
}

bool EditorUtilities::GetApplicationImage(const Guid& imageId, TextureData& imageData, ApplicationImageType type)
{
    AssetReference<Texture> icon = Content::LoadAsync<Texture>(imageId);
    if (icon == nullptr)
    {
        const auto gameSettings = GameSettings::Get();
        if (gameSettings)
        {
            icon = Content::LoadAsync<Texture>(gameSettings->Icon);
        }
    }
    if (icon == nullptr)
    {
        if (type == ApplicationImageType::Icon)
        {
            icon = Content::LoadAsyncInternal<Texture>(TEXT("Engine/Textures/FlaxIconBlue"));
        }
        else if (type == ApplicationImageType::SplashScreen)
        {
            icon = Content::LoadAsyncInternal<Texture>(TEXT("Engine/Textures/SplashScreenLogo"));
        }
    }
    if (icon)
    {
        return GetTexture(icon.GetID(), imageData);
    }
    return true;
}

bool EditorUtilities::GetTexture(const Guid& textureId, TextureData& textureData)
{
    AssetReference<Texture> texture = Content::LoadAsync<Texture>(textureId);
    if (texture)
    {
        if (texture->WaitForLoaded())
        {
            LOG(Warning, "Waiting for the texture to be loaded failed.");
        }
        else
        {
            const bool useGPU = texture->IsVirtual();
            if (useGPU)
            {
                int32 waits = 1000;
                const auto targetResidency = texture->StreamingTexture()->GetMaxResidency();
                ASSERT(targetResidency > 0);
                while (targetResidency != texture->StreamingTexture()->GetCurrentResidency() && waits-- > 0)
                {
                    Platform::Sleep(10);
                }

                // Get texture data from GPU
                if (!texture->GetTexture()->DownloadData(textureData))
                    return false;
            }

            // Get texture data from asset
            if (!texture->GetTextureData(textureData))
                return false;

            LOG(Warning, "Loading texture data failed.");
        }
    }
    return true;
}

bool EditorUtilities::ExportApplicationImage(const Guid& iconId, int32 width, int32 height, PixelFormat format, const String& path, ApplicationImageType type)
{
    TextureData icon;
    if (GetApplicationImage(iconId, icon, type))
        return true;

    return ExportApplicationImage(icon, width, height, format, path);
}

bool EditorUtilities::ExportApplicationImage(const TextureData& icon, int32 width, int32 height, PixelFormat format, const String& path)
{
    // Change format if need to
    const TextureData* iconData = &icon;
    TextureData tmpData1, tmpData2;
    if (icon.Format != format)
    {
        // Pre-multiply alpha if has it to prevent strange colors appear
        if (PixelFormatExtensions::HasAlpha(iconData->Format) && !PixelFormatExtensions::HasAlpha(format))
        {
            // Pre-multiply alpha if can
            auto sampler = TextureTool::GetSampler(iconData->Format);
            if (!sampler)
            {
                if (TextureTool::Convert(tmpData2, *iconData, PixelFormat::R16G16B16A16_Float))
                {
                    LOG(Error, "Failed convert texture data.");
                    return true;
                }
                iconData = &tmpData2;
                sampler = TextureTool::GetSampler(iconData->Format);
            }
            if (sampler)
            {
                auto mipData = iconData->GetData(0, 0);
                for (int32 y = 0; y < iconData->Height; y++)
                {
                    for (int32 x = 0; x < iconData->Width; x++)
                    {
                        Color color = TextureTool::SamplePoint(sampler, x, y, mipData->Data.Get(), mipData->RowPitch);
                        color *= color.A;
                        color.A = 1.0f;
                        TextureTool::Store(sampler, x, y, mipData->Data.Get(), mipData->RowPitch, color);
                    }
                }
            }
        }
        if (TextureTool::Convert(tmpData1, *iconData, format))
        {
            LOG(Error, "Failed convert texture data.");
            return true;
        }
        iconData = &tmpData1;
    }

    // Resize if need to
    TextureData tmpData3;
    if (iconData->Width != width || iconData->Height != height)
    {
        if (PixelFormatExtensions::HasAlpha(icon.Format) && !PixelFormatExtensions::HasAlpha(format))
        {
            // Pre-multiply alpha if can
            auto sampler = TextureTool::GetSampler(icon.Format);
            if (sampler)
            {
                auto mipData = iconData->GetData(0, 0);
                for (int32 y = 0; y < iconData->Height; y++)
                {
                    for (int32 x = 0; x < iconData->Width; x++)
                    {
                        Color color = TextureTool::SamplePoint(sampler, x, y, mipData->Data.Get(), mipData->RowPitch);
                        color *= color.A;
                        color.A = 1.0f;
                        TextureTool::Store(sampler, x, y, mipData->Data.Get(), mipData->RowPitch, color);
                    }
                }
            }
        }
        if (TextureTool::Resize(tmpData3, *iconData, width, height))
        {
            LOG(Error, "Failed resize texture data.");
            return true;
        }
        iconData = &tmpData3;
    }

    // Save to file
    if (TextureTool::ExportTexture(path, *iconData))
    {
        LOG(Error, "Failed to save application icon.");
        return true;
    }

    return false;
}

bool EditorUtilities::FindWDKBin(String& outputWdkBinPath)
{
    // Find Windows Driver Kit (WDK) install location
    const Char* wdkPaths[] =
    {
        TEXT("C:\\Program Files (x86)\\Windows Kits\\10\\bin"),
        TEXT("C:\\Program Files (x86)\\Windows Kits\\10\\10.0.19041.0\\bin"),
        TEXT("C:\\Program Files (x86)\\Windows Kits\\10\\10.0.18362.0\\bin"),
        TEXT("C:\\Program Files (x86)\\Windows Kits\\10\\10.0.17763.0\\bin"),
        TEXT("C:\\Program Files (x86)\\Windows Kits\\10\\10.0.17134.0\\bin"),
        TEXT("C:\\Program Files (x86)\\Windows Kits\\10\\10.0.16299.0\\bin"),
        TEXT("C:\\Program Files (x86)\\Windows Kits\\10\\10.0.15063.0\\bin"),
        TEXT("C:\\Program Files (x86)\\Windows Kits\\10\\10.0.14393.0\\bin"),
        TEXT("C:\\Program Files (x86)\\Windows Kits\\8.1\\bin"),
        TEXT("C:\\Program Files\\Windows Kits\\10\\bin"),
        TEXT("C:\\Program Files\\Windows Kits\\8.1\\bin"),
    };

    for (int32 i = 0; i < ARRAY_COUNT(wdkPaths); i++)
    {
        if (FileSystem::DirectoryExists(wdkPaths[i]))
        {
            outputWdkBinPath = wdkPaths[i];
            return false;
        }
    }

    return true;
}

bool EditorUtilities::GenerateCertificate(const String& name, const String& outputPfxFilePath)
{
    // Generate temp files paths
    const auto id = Guid::New().ToString(Guid::FormatType::D);
    auto outputCerFilePath = Globals::TemporaryFolder / id;
    const auto outputPvkFilePath = outputCerFilePath + TEXT(".pvk");
    outputCerFilePath += TEXT(".cer");

    // Generate .pfx file
    const bool result = GenerateCertificate(name, outputPfxFilePath, outputCerFilePath, outputPvkFilePath);

    // Cleanup unused files
    if (FileSystem::FileExists(outputCerFilePath))
        FileSystem::DeleteFile(outputCerFilePath);
    if (FileSystem::FileExists(outputPvkFilePath))
        FileSystem::DeleteFile(outputPvkFilePath);

    return result;
}

bool EditorUtilities::GenerateCertificate(const String& name, const String& outputPfxFilePath, const String& outputCerFilePath, const String& outputPvkFilePath)
{
    String wdkPath;
    if (FindWDKBin(wdkPath))
    {
        LOG(Warning, "Cannot find WDK install location.");
        return true;
    }
    wdkPath /= TEXT("x86\\");

    // MakeCert
    auto path = wdkPath / TEXT("makecert.exe");
    CreateProcessSettings procSettings;
    procSettings.FileName = String::Format(TEXT("\"{0}\" /r /h 0 /eku \"1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13\" /m 12 /len 2048 /n \"CN={1}\" -sv \"{2}\" \"{3}\""), path, name, outputPvkFilePath, outputCerFilePath);
    int32 result = Platform::CreateProcess(procSettings);
    if (result != 0)
    {
        LOG(Warning, "MakeCert failed with result {0}.", result);
        return true;
    }

    // Pvk2Pfx
    path = wdkPath / TEXT("pvk2pfx.exe");
    procSettings.FileName = String::Format(TEXT("\"{0}\" -pvk \"{1}\" -spc \"{2}\" -pfx \"{3}\""), path, outputPvkFilePath, outputCerFilePath, outputPfxFilePath);
    result = Platform::CreateProcess(procSettings);
    if (result != 0)
    {
        LOG(Warning, "MakeCert failed with result {0}.", result);
        return true;
    }

    return false;
}

bool EditorUtilities::IsInvalidPathChar(Char c)
{
    char illegalChars[] =
    {
        '?',
        '\\',
        '/',
        '\"',
        '<',
        '>',
        '|',
        ':',
        '*',
        '\u0001',
        '\u0002',
        '\u0003',
        '\u0004',
        '\u0005',
        '\u0006',
        '\a',
        '\b',
        '\t',
        '\n',
        '\v',
        '\f',
        '\r',
        '\u000E',
        '\u000F',
        '\u0010',
        '\u0011',
        '\u0012',
        '\u0013',
        '\u0014',
        '\u0015',
        '\u0016',
        '\u0017',
        '\u0018',
        '\u0019',
        '\u001A',
        '\u001B',
        '\u001C',
        '\u001D',
        '\u001E',
        '\u001F'
    };
    for (auto i : illegalChars)
    {
        if (c == i)
            return true;
    }
    return false;
}

void EditorUtilities::ValidatePathChars(String& filename, char invalidCharReplacement)
{
    if (invalidCharReplacement == 0)
    {
        StringBuilder result;
        for (int32 i = 0; i < filename.Length(); i++)
        {
            if (!IsInvalidPathChar(filename[i]))
                result.Append(filename[i]);
        }
        filename = result.ToString();
    }
    else
    {
        for (int32 i = 0; i < filename.Length(); i++)
        {
            if (IsInvalidPathChar(filename[i]))
                filename[i] = invalidCharReplacement;
        }
    }
}

bool EditorUtilities::ReplaceInFiles(const String& folderPath, const Char* searchPattern, DirectorySearchOption searchOption, const String& findWhat, const String& replaceWith)
{
    Array<String> files;
    FileSystem::DirectoryGetFiles(files, folderPath, searchPattern, searchOption);
    for (auto& file : files)
        if (ReplaceInFile(file, findWhat, replaceWith))
            return true;
    return false;
}

bool EditorUtilities::ReplaceInFile(const StringView& file, const StringView& findWhat, const StringView& replaceWith)
{
    String text;
    if (File::ReadAllText(file, text))
        return true;
    text.Replace(findWhat.Get(), findWhat.Length(), replaceWith.Get(), replaceWith.Length());
    return File::WriteAllText(file, text, Encoding::ANSI);
}

bool EditorUtilities::ReplaceInFile(const StringView& file, const Dictionary<String, String, HeapAllocation>& replaceMap)
{
    String text;
    if (File::ReadAllText(file, text))
        return true;
    for (const auto& e : replaceMap)
        text.Replace(e.Key.Get(), e.Key.Length(), e.Value.Get(), e.Value.Length());
    return File::WriteAllText(file, text, Encoding::ANSI);
}

bool EditorUtilities::CopyFileIfNewer(const StringView& dst, const StringView& src)
{
    if (FileSystem::FileExists(dst) &&
        FileSystem::GetFileLastEditTime(src) <= FileSystem::GetFileLastEditTime(dst) &&
        FileSystem::GetFileSize(dst) == FileSystem::GetFileSize(src))
        return false;
    return FileSystem::CopyFile(dst, src);
}

bool EditorUtilities::CopyDirectoryIfNewer(const StringView& dst, const StringView& src, bool withSubDirectories)
{
    if (FileSystem::DirectoryExists(dst))
    {
        // Copy all files
        Array<String> cache(32);
        if (FileSystem::DirectoryGetFiles(cache, *src, TEXT("*"), DirectorySearchOption::TopDirectoryOnly))
            return true;
        for (int32 i = 0; i < cache.Count(); i++)
        {
            String dstFile = String(dst) / StringUtils::GetFileName(cache[i]);
            if (CopyFileIfNewer(*dstFile, *cache[i]))
                return true;
        }

        // Copy all subdirectories (if need to)
        if (withSubDirectories)
        {
            cache.Clear();
            if (FileSystem::GetChildDirectories(cache, src))
                return true;
            for (int32 i = 0; i < cache.Count(); i++)
            {
                String dstDir = String(dst) / StringUtils::GetFileName(cache[i]);
                if (CopyDirectoryIfNewer(dstDir, cache[i], true))
                    return true;
            }
        }

        return false;
    }
    else
    {
        return FileSystem::CopyDirectory(dst, src, withSubDirectories);
    }
}
