// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "EditorUtilities.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Texture.h"
#include <fstream>

#define MSDOS_SIGNATURE 0x5A4D
#define PE_SIGNATURE 0x00004550
#define PE_32BIT_SIGNATURE 0x10B
#define PE_64BIT_SIGNATURE 0x20B
#define PE_NUM_DIRECTORY_ENTRIES 16
#define PE_SECTION_UNINITIALIZED_DATA 0x00000080
#define PE_IMAGE_DIRECTORY_ENTRY_RESOURCE 2
#define PE_IMAGE_RT_ICON 3

/// <summary>
/// MS-DOS header found at the beginning in a PE format file.
/// </summary>
struct MSDOSHeader
{
    uint16 signature;
    uint16 lastSize;
    uint16 numBlocks;
    uint16 numReloc;
    uint16 hdrSize;
    uint16 minAlloc;
    uint16 maxAlloc;
    uint16 ss;
    uint16 sp;
    uint16 checksum;
    uint16 ip;
    uint16 cs;
    uint16 relocPos;
    uint16 numOverlay;
    uint16 reserved1[4];
    uint16 oemId;
    uint16 oemInfo;
    uint16 reserved2[10];
    uint32 lfanew;
};

/// <summary>
/// COFF header found in a PE format file.
/// </summary>
struct COFFHeader
{
    uint16 machine;
    uint16 numSections;
    uint32 timeDateStamp;
    uint32 ptrSymbolTable;
    uint32 numSymbols;
    uint16 sizeOptHeader;
    uint16 characteristics;
};

/// <summary>
/// Contains address and size of data areas in a PE image.
/// </summary>
struct PEDataDirectory
{
    uint32 virtualAddress;
    uint32 size;
};

/// <summary>
/// Optional header in a 32-bit PE format file.
/// </summary>
struct PEOptionalHeader32
{
    uint16 signature;
    uint8 majorLinkerVersion;
    uint8 minorLinkerVersion;
    uint32 sizeCode;
    uint32 sizeInitializedData;
    uint32 sizeUninitializedData;
    uint32 addressEntryPoint;
    uint32 baseCode;
    uint32 baseData;
    uint32 baseImage;
    uint32 alignmentSection;
    uint32 alignmentFile;
    uint16 majorOSVersion;
    uint16 minorOSVersion;
    uint16 majorImageVersion;
    uint16 minorImageVersion;
    uint16 majorSubsystemVersion;
    uint16 minorSubsystemVersion;
    uint32 reserved;
    uint32 sizeImage;
    uint32 sizeHeaders;
    uint32 checksum;
    uint16 subsystem;
    uint16 characteristics;
    uint32 sizeStackReserve;
    uint32 sizeStackCommit;
    uint32 sizeHeapReserve;
    uint32 sizeHeapCommit;
    uint32 loaderFlags;
    uint32 NumRvaAndSizes;
    PEDataDirectory dataDirectory[16];
};

/// <summary>
/// Optional header in a 64-bit PE format file.
/// </summary>
struct PEOptionalHeader64
{
    uint16 signature;
    uint8 majorLinkerVersion;
    uint8 minorLinkerVersion;
    uint32 sizeCode;
    uint32 sizeInitializedData;
    uint32 sizeUninitializedData;
    uint32 addressEntryPoint;
    uint32 baseCode;
    uint64 baseImage;
    uint32 alignmentSection;
    uint32 alignmentFile;
    uint16 majorOSVersion;
    uint16 minorOSVersion;
    uint16 majorImageVersion;
    uint16 minorImageVersion;
    uint16 majorSubsystemVersion;
    uint16 minorSubsystemVersion;
    uint32 reserved;
    uint32 sizeImage;
    uint32 sizeHeaders;
    uint32 checksum;
    uint16 subsystem;
    uint16 characteristics;
    uint64 sizeStackReserve;
    uint64 sizeStackCommit;
    uint64 sizeHeapReserve;
    uint64 sizeHeapCommit;
    uint32 loaderFlags;
    uint32 NumRvaAndSizes;
    PEDataDirectory dataDirectory[16];
};

/// <summary>
/// A section header in a PE format file.
/// </summary>
struct PESectionHeader
{
    char name[8];
    uint32 virtualSize;
    uint32 relativeVirtualAddress;
    uint32 physicalSize;
    uint32 physicalAddress;
    uint8 deprecated[12];
    uint32 flags;
};

/// <summary>
/// A resource table header within a .rsrc section in a PE format file.
/// </summary>
struct PEImageResourceDirectory
{
    uint32 flags;
    uint32 timeDateStamp;
    uint16 majorVersion;
    uint16 minorVersion;
    uint16 numNamedEntries;
    uint16 numIdEntries;
};

/// <summary>
/// A single entry in a resource table within a .rsrc section in a PE format file.
/// </summary>
struct PEImageResourceEntry
{
    uint32 type;
    uint32 offsetDirectory : 31;
    uint32 isDirectory : 1;
};

/// <summary>
/// An entry in a resource table referencing resource data. Found within a .rsrc section in a PE format file.
/// </summary>
struct PEImageResourceEntryData
{
    uint32 offsetData;
    uint32 size;
    uint32 codePage;
    uint32 resourceHandle;
};

/// <summary>
/// Header used in icon file format. 
/// </summary>
struct IconHeader
{
    uint32 size;
    int32 width;
    int32 height;
    uint16 planes;
    uint16 bitCount;
    uint32 compression;
    uint32 sizeImage;
    int32 xPelsPerMeter;
    int32 yPelsPerMeter;
    uint32 clrUsed;
    uint32 clrImportant;
};

void UpdateIconData(uint8* iconData, const TextureData* icon)
{
    IconHeader* iconHeader = (IconHeader*)iconData;

    if (iconHeader->size != sizeof(IconHeader) || iconHeader->compression != 0 || iconHeader->planes != 1 || iconHeader->bitCount != 32)
    {
        // Unsupported format
        return;
    }

    uint8* iconPixels = iconData + sizeof(IconHeader);
    uint32 width = iconHeader->width;
    uint32 height = iconHeader->height / 2;

    // Check if can use mip from texture data or sample different mip
    uint32 iconTexSize;
    if (width != height)
    {
        // Only square icons are supported
        return;
    }
    if (Math::IsPowerOfTwo(width))
    {
        // Use mip
        iconTexSize = width;
    }
    else
    {
        // Use resized mip
        iconTexSize = Math::RoundUpToPowerOf2(width);
    }

    // Try to pick a proper mip (require the same size)
    const TextureMipData* srcPixels = nullptr;
    int32 mipLevels = icon->GetMipLevels();
    for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
    {
        uint32 iconWidth = Math::Max(1, icon->Width >> mipIndex);
        uint32 iconHeight = Math::Max(1, icon->Height >> mipIndex);

        if (iconTexSize == iconWidth && iconTexSize == iconHeight)
        {
            srcPixels = icon->GetData(0, mipIndex);
            break;
        }
    }
    if (srcPixels == nullptr)
    {
        // No icon of this size provided
        return;
    }

    // Write colors
    uint32* colorData = (uint32*)iconPixels;

    uint32 idx = 0;
    for (int32 y = (int32)height - 1; y >= 0; y--)
    {
        float v = (float)y / height;
        for (uint32 x = 0; x < width; x++)
        {
            float u = (float)x / width;

            int32 i = (int32)(v * iconTexSize) * srcPixels->RowPitch + (int32)(u * iconTexSize) * sizeof(Color32);
            colorData[idx++] = ((Color32*)&srcPixels->Data.Get()[i])->GetAsBGRA();
        }
    }

    // Write AND mask
    uint32 colorDataSize = width * height * sizeof(uint32);
    uint8* maskData = iconPixels + colorDataSize;

    // One per bit in byte
    uint32 numPackedPixels = width / 8;

    for (int32 y = (int32)height - 1; y >= 0; y--)
    {
        uint8 mask = 0;
        float v = (float)y / height;
        for (uint32 packedX = 0; packedX < numPackedPixels; packedX++)
        {
            for (uint32 pixelIdx = 0; pixelIdx < 8; pixelIdx++)
            {
                uint32 x = packedX * 8 + pixelIdx;
                float u = (float)x / width;
                int32 i = (int32)(v * iconTexSize) * srcPixels->RowPitch + (int32)(u * iconTexSize) * sizeof(Color32);
                Color32 color = *((Color32*)&srcPixels->Data.Get()[i]);
                if (color.A < 64)
                    mask |= 1 << (7 - pixelIdx);
            }

            *maskData = mask;
            maskData++;
        }
    }
}

void SetIconData(PEImageResourceDirectory* base, PEImageResourceDirectory* current, uint8* imageData, uint32 sectionAddress, const TextureData* iconRGBA8)
{
    uint32 numEntries = current->numIdEntries; // Not supporting name entries
    PEImageResourceEntry* entries = (PEImageResourceEntry*)(current + 1);

    for (uint32 i = 0; i < numEntries; i++)
    {
        // Only at root does the type identify resource type
        if (base == current && entries[i].type != PE_IMAGE_RT_ICON)
            continue;

        if (entries[i].isDirectory)
        {
            PEImageResourceDirectory* child = (PEImageResourceDirectory*)(((uint8*)base) + entries[i].offsetDirectory);
            SetIconData(base, child, imageData, sectionAddress, iconRGBA8);
        }
        else
        {
            PEImageResourceEntryData* data = (PEImageResourceEntryData*)(((uint8*)base) + entries[i].offsetDirectory);

            uint8* iconData = imageData + (data->offsetData - sectionAddress);
            UpdateIconData(iconData, iconRGBA8);
        }
    }
}

bool EditorUtilities::UpdateExeIcon(const String& path, const TextureData& icon)
{
    // Validate input
    if (!FileSystem::FileExists(path))
    {
        LOG(Warning, "Missing file");
        return true;
    }
    if (icon.Width < 1 || icon.Height < 1 || icon.GetMipLevels() <= 0)
    {
        LOG(Warning, "Inalid icon data");
        return true;
    }

    // Convert to RGBA8 format if need to
    const TextureData* iconRGBA8 = &icon;
    TextureData tmpData1;
    if (icon.Format != PixelFormat::R8G8B8A8_UNorm)
    {
        if (TextureTool::Convert(tmpData1, *iconRGBA8, PixelFormat::R8G8B8A8_UNorm))
        {
            LOG(Warning, "Failed convert icon data.");
            return true;
        }
        iconRGBA8 = &tmpData1;
    }

    // Resize if need to
    TextureData tmpData2;
    if (iconRGBA8->Width > 256 || iconRGBA8->Height > 256)
    {
        if (TextureTool::Resize(tmpData2, *iconRGBA8, 256, 256))
        {
            LOG(Warning, "Failed resize icon data.");
            return true;
        }
        iconRGBA8 = &tmpData2;
    }

    // A PE file is structured as such:
    //  - MSDOS Header
    //  - PE Signature
    //  - COFF Header
    //  - PE Optional Header
    //  - One or multiple sections
    //   - .code
    //   - .data
    //   - ...
    //   - .rsrc
    //    - icon/cursor/etc data

    std::fstream stream;
    stream.open(path.Get(), std::ios::in | std::ios::out | std::ios::binary);
    if (!stream.is_open())
    {
        LOG(Warning, "Cannot open file");
        return true;
    }

    // First check magic number to ensure file is even an executable
    uint16 magicNum;
    stream.read((char*)&magicNum, sizeof(magicNum));
    if (magicNum != MSDOS_SIGNATURE)
    {
        LOG(Warning, "Provided file is not a valid executable.");
        return true;
    }

    // Read the MSDOS header and skip over it
    stream.seekg(0);

    MSDOSHeader msdosHeader;
    stream.read((char*)&msdosHeader, sizeof(MSDOSHeader));

    // Read PE signature
    stream.seekg(msdosHeader.lfanew);

    uint32 peSignature;
    stream.read((char*)&peSignature, sizeof(peSignature));

    if (peSignature != PE_SIGNATURE)
    {
        LOG(Warning, "Provided file is not in PE format.");
        return true;
    }

    // Read COFF header
    COFFHeader coffHeader;
    stream.read((char*)&coffHeader, sizeof(COFFHeader));

    // .exe files always have an optional header
    if (coffHeader.sizeOptHeader == 0)
    {
        LOG(Warning, "Provided file is not a valid executable.");
        return true;
    }

    uint32 numSectionHeaders = coffHeader.numSections;

    // Read optional header
    auto optionalHeaderPos = stream.tellg();

    uint16 optionalHeaderSignature;
    stream.read((char*)&optionalHeaderSignature, sizeof(optionalHeaderSignature));

    PEDataDirectory* dataDirectory = nullptr;
    stream.seekg(optionalHeaderPos);
    if (optionalHeaderSignature == PE_32BIT_SIGNATURE)
    {
        PEOptionalHeader32 optionalHeader;
        stream.read((char*)&optionalHeader, sizeof(optionalHeader));

        dataDirectory = optionalHeader.dataDirectory + PE_IMAGE_DIRECTORY_ENTRY_RESOURCE;
    }
    else if (optionalHeaderSignature == PE_64BIT_SIGNATURE)
    {
        PEOptionalHeader64 optionalHeader;
        stream.read((char*)&optionalHeader, sizeof(optionalHeader));

        dataDirectory = optionalHeader.dataDirectory + PE_IMAGE_DIRECTORY_ENTRY_RESOURCE;
    }
    else
    {
        LOG(Warning, "Unrecognized PE format.");
        return true;
    }

    // Read section headers
    auto sectionHeaderPos = optionalHeaderPos + (std::ifstream::pos_type)coffHeader.sizeOptHeader;
    stream.seekg(sectionHeaderPos);

    Array<PESectionHeader> sectionHeaders;
    sectionHeaders.Resize(numSectionHeaders);
    stream.read((char*)sectionHeaders.Get(), sizeof(PESectionHeader) * numSectionHeaders);

    // Look for .rsrc section header
    for (uint32 i = 0; i < numSectionHeaders; i++)
    {
        if (sectionHeaders[i].flags & PE_SECTION_UNINITIALIZED_DATA)
            continue;

        if (strcmp(sectionHeaders[i].name, ".rsrc") == 0)
        {
            uint32 imageSize = sectionHeaders[i].physicalSize;
            Array<uint8> imageData;
            imageData.Resize(imageSize);

            stream.seekg(sectionHeaders[i].physicalAddress);
            stream.read((char*)imageData.Get(), imageSize);

            uint32 resourceDirOffset = dataDirectory->virtualAddress - sectionHeaders[i].relativeVirtualAddress;
            PEImageResourceDirectory* resourceDirectory = (PEImageResourceDirectory*)&imageData.Get()[resourceDirOffset];

            SetIconData(resourceDirectory, resourceDirectory, imageData.Get(), sectionHeaders[i].relativeVirtualAddress, iconRGBA8);
            stream.seekp(sectionHeaders[i].physicalAddress);
            stream.write((char*)imageData.Get(), imageSize);
        }
    }

    stream.close();

    return false;
}

bool EditorUtilities::GetApplicationImage(const Guid& imageId, TextureData& imageData, ApplicationImageType type)
{
    AssetReference<Texture> icon = Content::LoadAsync<Texture>(imageId);
    if (icon == nullptr)
    {
        icon = Content::LoadAsync<Texture>(GameSettings::Icon);
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
        // Note: this could load texture asset data but using Texture API is easier

        if (texture->WaitForLoaded())
        {
            LOG(Warning, "Waiting for the texture to be loaded failed.");
        }
        else
        {
            // TODO: disable streaming for a texture or set max quality override

            int32 waits = 1000;
            const auto targetResidency = texture->StreamingTexture()->GetMaxResidency();
            ASSERT(targetResidency > 0);
            while (targetResidency != texture->StreamingTexture()->GetCurrentResidency() && waits-- > 0)
            {
                Platform::Sleep(10);
            }

            ASSERT(texture->IsLoaded());
            if (texture->GetTexture()->DownloadData(textureData))
            {
                LOG(Warning, "Loading texture data failed.");
            }
            else
            {
                return false;
            }
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
    auto args = String::Format(TEXT("\"{0}\" /r /h 0 /eku \"1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13\" /m 12 /len 2048 /n \"CN={1}\" -sv \"{2}\" \"{3}\""), path, name, outputPvkFilePath, outputCerFilePath);
    int32 result = Platform::RunProcess(args, String::Empty);
    if (result != 0)
    {
        LOG(Warning, "MakeCert failed with result {0}.", result);
        return true;
    }

    // Pvk2Pfx
    path = wdkPath / TEXT("pvk2pfx.exe");
    args = String::Format(TEXT("\"{0}\" -pvk \"{1}\" -spc \"{2}\" -pfx \"{3}\""), path, outputPvkFilePath, outputCerFilePath, outputPfxFilePath);
    result = Platform::RunProcess(args, String::Empty);
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
    text.Replace(findWhat.GetText(), replaceWith.GetText());
    return File::WriteAllText(file, text, Encoding::ANSI);
}
