// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_WINDOWS

#include "WindowsPlatformTools.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Windows/WindowsPlatformSettings.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Graphics/PixelFormatSampler.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include <fstream>
#include <winver.h>

#define MSDOS_SIGNATURE 0x5A4D
#define PE_SIGNATURE 0x00004550
#define PE_32BIT_SIGNATURE 0x10B
#define PE_64BIT_SIGNATURE 0x20B
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
    const uint32 width = iconHeader->width;
    const uint32 height = iconHeader->height / 2;

    // Try to pick a proper mip (require the same size)
    int32 srcPixelsMip = 0;
    const int32 mipLevels = icon->GetMipLevels();
    for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
    {
        const uint32 iconWidth = Math::Max(1, icon->Width >> mipIndex);
        const uint32 iconHeight = Math::Max(1, icon->Height >> mipIndex);
        if (width == iconWidth && height == iconHeight)
        {
            srcPixelsMip = mipIndex;
            break;
        }
    }
    const TextureMipData* srcPixels = icon->GetData(0, srcPixelsMip);
    const Color32* srcPixelsData = (Color32*)srcPixels->Data.Get();
    const Int2 srcPixelsSize(Math::Max(1, icon->Width >> srcPixelsMip), Math::Max(1, icon->Height >> srcPixelsMip));
    const auto sampler = PixelFormatSampler::Get(icon->Format);
    ASSERT_LOW_LAYER(sampler);

    // Write colors
    uint32* colorData = (uint32*)iconPixels;
    uint32 idx = 0;
    for (int32 y = (int32)height - 1; y >= 0; y--)
    {
        float v = (float)y / height;
        for (uint32 x = 0; x < width; x++)
        {
            float u = (float)x / width;
            const Color c = sampler->SampleLinear(srcPixelsData, Float2(u, v), srcPixelsSize, srcPixels->RowPitch);
            colorData[idx++] = Color32(c).GetAsBGRA();
        }
    }

    // Write AND mask
    uint32 colorDataSize = width * height * sizeof(uint32);
    uint8* maskData = iconPixels + colorDataSize;
    uint32 numPackedPixels = width / 8; // One per bit in byte
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
                const Color c = sampler->SampleLinear(srcPixelsData, Float2(u, v), srcPixelsSize, srcPixels->RowPitch);
                if (c.A < 0.25f)
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

bool UpdateExeIcon(const String& path, const TextureData& icon)
{
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

    // Ensure that image format can be sampled
    const TextureData* iconRGBA8 = &icon;
    TextureData tmpData1;
    //if (icon.Format != PixelFormat::R8G8B8A8_UNorm)
    if (PixelFormatSampler::Get(icon.Format) == nullptr)
    {
        if (TextureTool::Convert(tmpData1, *iconRGBA8, PixelFormat::R8G8B8A8_UNorm))
        {
            LOG(Warning, "Failed convert icon data.");
            return true;
        }
        iconRGBA8 = &tmpData1;
    }

    // Use fixed-size input icon image
    TextureData tmpData2;
    if (iconRGBA8->Width != 256 || iconRGBA8->Height != 256)
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
#if PLATFORM_WINDOWS
    stream.open(path.Get(), std::ios::in | std::ios::out | std::ios::binary);
#else
    StringAsANSI<> pathAnsi(path.Get());
    stream.open(pathAnsi.Get(), std::ios::in | std::ios::out | std::ios::binary);
#endif
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
    if (coffHeader.sizeOptHeader == 0)
    {
        LOG(Warning, "Provided file is not a valid executable.");
        return true;
    }
    uint32 sectionHeadersCount = coffHeader.numSections;

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
    sectionHeaders.Resize(sectionHeadersCount);
    stream.read((char*)sectionHeaders.Get(), sizeof(PESectionHeader) * sectionHeadersCount);

    // Look for .rsrc section header
    for (uint32 i = 0; i < sectionHeadersCount; i++)
    {
        PESectionHeader& sectionHeader = sectionHeaders[i];
        if (sectionHeader.flags & PE_SECTION_UNINITIALIZED_DATA)
            continue;
        if (strcmp(sectionHeader.name, ".rsrc") == 0)
        {
            uint32 imageSize = sectionHeader.physicalSize;
            Array<uint8> imageData;
            imageData.Resize(imageSize);

            stream.seekg(sectionHeader.physicalAddress);
            stream.read((char*)imageData.Get(), imageSize);

            uint32 resourceDirOffset = dataDirectory->virtualAddress - sectionHeader.relativeVirtualAddress;
            PEImageResourceDirectory* resourceDirectory = (PEImageResourceDirectory*)&imageData.Get()[resourceDirOffset];

            SetIconData(resourceDirectory, resourceDirectory, imageData.Get(), sectionHeader.relativeVirtualAddress, iconRGBA8);
            stream.seekp(sectionHeader.physicalAddress);
            stream.write((char*)imageData.Get(), imageSize);
        }
    }

    stream.close();

    return false;
}

namespace
{
    void Pad4(MemoryWriteStream& stream)
    {
        while ((stream.GetPosition() % 4) != 0)
        {
            const byte zero = 0;
            stream.WriteBytes(&zero, 1);
        }
    }

    void WriteVersionString(MemoryWriteStream& stream, const Char* key, const String& value)
    {
        const uint32 startPos = stream.GetPosition();
        const uint16 dummyLength = 0;
        stream.WriteBytes(&dummyLength, sizeof(dummyLength));

        const uint16 valueLengthChars = value.IsEmpty() ? 0 : (uint16)(value.Length() + 1);
        stream.WriteBytes(&valueLengthChars, sizeof(valueLengthChars));

        const uint16 wType = 1; // Text
        stream.WriteBytes(&wType, sizeof(wType));

        const int32 keyLenBytes = (StringUtils::Length(key) + 1) * sizeof(Char);
        stream.WriteBytes(key, keyLenBytes);
        Pad4(stream);

        if (!value.IsEmpty())
        {
            const int32 valLenBytes = (value.Length() + 1) * sizeof(Char);
            stream.WriteBytes(value.GetText(), valLenBytes);
            Pad4(stream);
        }

        const uint32 endPos = stream.GetPosition();
        const uint16 totalLength = (uint16)(endPos - startPos);
        stream.SetPosition(startPos);
        stream.WriteBytes(&totalLength, sizeof(totalLength));
        stream.SetPosition(endPos);
    }

    void BuildVersionResource(MemoryWriteStream& stream, int32 major, int32 minor, int32 build, int32 revision, bool isDebug, const Dictionary<String, String>& stringTable, uint16 langId = 0x0400, uint16 codePage = 1200)
    {
        // VS_VERSIONINFO
        const uint32 rootStart = stream.GetPosition();
        const uint16 dummy = 0;
        stream.WriteBytes(&dummy, sizeof(dummy)); // wLength placeholder

        const uint16 fixedInfoSize = sizeof(VS_FIXEDFILEINFO);
        stream.WriteBytes(&fixedInfoSize, sizeof(fixedInfoSize));

        const uint16 rootType = 0; // Binary
        stream.WriteBytes(&rootType, sizeof(rootType));

        const Char keyVsVersionInfo[] = TEXT("VS_VERSION_INFO");
        stream.WriteBytes(keyVsVersionInfo, sizeof(keyVsVersionInfo));
        Pad4(stream);

        // VS_FIXEDFILEINFO
        VS_FIXEDFILEINFO fixedInfo;
        Platform::MemoryClear(&fixedInfo, sizeof(fixedInfo));
        fixedInfo.dwSignature = 0xFEEF04BD;
        fixedInfo.dwStrucVersion = 0x00010000;
        fixedInfo.dwFileVersionMS = ((uint32)major << 16) | ((uint32)minor & 0xFFFF);
        fixedInfo.dwFileVersionLS = ((uint32)Math::Max(build, 0) << 16) | ((uint32)Math::Max(revision, 0) & 0xFFFF);
        fixedInfo.dwProductVersionMS = ((uint32)major << 16) | ((uint32)minor & 0xFFFF);
        fixedInfo.dwProductVersionLS = ((uint32)Math::Max(build, 0) << 16) | ((uint32)Math::Max(revision, 0) & 0xFFFF);
        fixedInfo.dwFileFlagsMask = 0x3F;
        fixedInfo.dwFileFlags = isDebug ? 1 : 0;
        fixedInfo.dwFileOS = 0x00040004; // VOS_NT_WINDOWS32
        fixedInfo.dwFileType = 1; // VFT_APP
        stream.WriteBytes(&fixedInfo, sizeof(fixedInfo));
        Pad4(stream);

        // StringFileInfo
        const uint32 sfiStart = stream.GetPosition();
        stream.WriteBytes(&dummy, sizeof(dummy));
        stream.WriteBytes(&dummy, sizeof(dummy));
        const uint16 textType = 1;
        stream.WriteBytes(&textType, sizeof(textType));

        const Char keyStringFileInfo[] = TEXT("StringFileInfo");
        stream.WriteBytes(keyStringFileInfo, sizeof(keyStringFileInfo));
        Pad4(stream);

        // StringTable
        const uint32 stStart = stream.GetPosition();
        stream.WriteBytes(&dummy, sizeof(dummy));
        stream.WriteBytes(&dummy, sizeof(dummy));
        stream.WriteBytes(&textType, sizeof(textType));

        const Char tableKey[] = TEXT("040004b0");
        stream.WriteBytes(tableKey, sizeof(tableKey));
        Pad4(stream);

        // Strings
        for (auto it = stringTable.Begin(); it != stringTable.End(); ++it)
        {
            WriteVersionString(stream, it->Key.GetText(), it->Value);
        }

        // Update StringTable length
        const uint32 stEnd = stream.GetPosition();
        const uint16 stLength = (uint16)(stEnd - stStart);
        stream.SetPosition(stStart);
        stream.WriteBytes(&stLength, sizeof(stLength));
        stream.SetPosition(stEnd);
        Pad4(stream);

        // Update StringFileInfo length
        const uint32 sfiEnd = stream.GetPosition();
        const uint16 sfiLength = (uint16)(sfiEnd - sfiStart);
        stream.SetPosition(sfiStart);
        stream.WriteBytes(&sfiLength, sizeof(sfiLength));
        stream.SetPosition(sfiEnd);
        Pad4(stream);

        // VarFileInfo
        const uint32 vfiStart = stream.GetPosition();
        stream.WriteBytes(&dummy, sizeof(dummy));
        stream.WriteBytes(&dummy, sizeof(dummy));
        stream.WriteBytes(&textType, sizeof(textType));

        const Char keyVarFileInfo[] = TEXT("VarFileInfo");
        stream.WriteBytes(keyVarFileInfo, sizeof(keyVarFileInfo));
        Pad4(stream);

        // Var (Translation)
        const uint32 varStart = stream.GetPosition();
        stream.WriteBytes(&dummy, sizeof(dummy));
        const uint16 varValueLength = 4; // 4 bytes for DWORD translation
        stream.WriteBytes(&varValueLength, sizeof(varValueLength));
        const uint16 binType = 0;
        stream.WriteBytes(&binType, sizeof(binType));

        const Char keyTranslation[] = TEXT("Translation");
        stream.WriteBytes(keyTranslation, sizeof(keyTranslation));
        Pad4(stream);
        stream.WriteBytes(&langId, sizeof(langId));
        stream.WriteBytes(&codePage, sizeof(codePage));
        Pad4(stream);

        // Update Var length
        const uint32 varEnd = stream.GetPosition();
        const uint16 varLength = (uint16)(varEnd - varStart);
        stream.SetPosition(varStart);
        stream.WriteBytes(&varLength, sizeof(varLength));
        stream.SetPosition(varEnd);
        Pad4(stream);

        // Update VarFileInfo length
        const uint32 vfiEnd = stream.GetPosition();
        const uint16 vfiLength = (uint16)(vfiEnd - vfiStart);
        stream.SetPosition(vfiStart);
        stream.WriteBytes(&vfiLength, sizeof(vfiLength));
        stream.SetPosition(vfiEnd);
        Pad4(stream);

        // Update VS_VERSIONINFO length
        const uint32 rootEnd = stream.GetPosition();
        const uint16 rootLength = (uint16)(rootEnd - rootStart);
        stream.SetPosition(rootStart);
        stream.WriteBytes(&rootLength, sizeof(rootLength));
        stream.SetPosition(rootEnd);
    }

    BOOL CALLBACK EnumResLangProc(HMODULE hModule, LPCWSTR lpszType, LPCWSTR lpszName, WORD wIDLanguage, LONG_PTR lParam)
    {
        auto* langList = (Array<WORD>*)lParam;
        langList->Add(wIDLanguage);
        return TRUE;
    }

    bool UpdateExeVersion(const String& path, CookingData& data, const WindowsPlatformSettings* platformSettings)
    {
        if (!FileSystem::FileExists(path))
        {
            LOG(Warning, "Missing file for executable version update.");
            return true;
        }

        const auto gameSettings = GameSettings::Get();
        const auto project = Editor::Project;

        String productName = gameSettings->ProductName;
        if (productName.IsEmpty() && project)
            productName = project->Name;
        if (productName.IsEmpty())
            productName = TEXT("Flax Game");

        String fileDescription = platformSettings ? platformSettings->Description : String::Empty;
        if (fileDescription.IsEmpty())
            fileDescription = productName;

        String companyName = gameSettings->CompanyName;
        if (companyName.IsEmpty() && project)
            companyName = project->Company;

        String copyright = gameSettings->CopyrightNotice;
        if (copyright.IsEmpty() && project)
            copyright = project->Copyright;

        String versionText = gameSettings->Version;
        Version version(1, 0, 0, 0);
        if (versionText.IsEmpty() && project)
        {
            version = project->Version;
            versionText = version.ToString();
        }
        else if (!versionText.IsEmpty())
        {
            if (Version::Parse(versionText, &version))
            {
                if (project)
                    version = project->Version;
            }
        }

        const String newName = EditorUtilities::GetOutputName();
        const String internalName = newName;
        const String originalFilename = newName + TEXT(".exe");

        Dictionary<String, String> stringTable;
        stringTable[TEXT("CompanyName")] = companyName;
        stringTable[TEXT("FileDescription")] = fileDescription;
        stringTable[TEXT("FileVersion")] = versionText;
        stringTable[TEXT("InternalName")] = internalName;
        stringTable[TEXT("LegalCopyright")] = copyright;
        stringTable[TEXT("OriginalFilename")] = originalFilename;
        stringTable[TEXT("ProductName")] = productName;
        stringTable[TEXT("ProductVersion")] = versionText;

        MemoryWriteStream resStream(2048);
        BuildVersionResource(resStream, version.Major(), version.Minor(), version.Build(), version.Revision(), data.Configuration == BuildConfiguration::Debug, stringTable, 0x0400, 1200);

        // Enumerate existing languages for RT_VERSION
        Array<WORD> oldLanguages;
        HMODULE hModule = LoadLibraryExW((LPCWSTR)path.GetText(), NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hModule)
        {
            EnumResourceLanguagesW(hModule, RT_VERSION, MAKEINTRESOURCEW(VS_VERSION_INFO), EnumResLangProc, (LONG_PTR)&oldLanguages);
            FreeLibrary(hModule);
        }

        HANDLE hUpdate = BeginUpdateResourceW((LPCWSTR)path.GetText(), FALSE);
        if (hUpdate == NULL)
        {
            LOG(Warning, "BeginUpdateResource failed with error 0x{0:x}", (uint32)GetLastError());
            return true;
        }

        // Delete existing version resources with different languages
        for (WORD oldLang : oldLanguages)
        {
            if (oldLang != 0x0400)
                UpdateResourceW(hUpdate, RT_VERSION, MAKEINTRESOURCEW(VS_VERSION_INFO), oldLang, NULL, 0);
        }

        // Add/update version resource (using 0x0400 - Process Default Language)
        if (!UpdateResourceW(hUpdate, RT_VERSION, MAKEINTRESOURCEW(VS_VERSION_INFO), 0x0400, resStream.GetHandle(), resStream.GetPosition()))
        {
            LOG(Warning, "UpdateResource failed with error 0x{0:x}", (uint32)GetLastError());
            EndUpdateResourceW(hUpdate, TRUE);
            return true;
        }

        if (!EndUpdateResourceW(hUpdate, FALSE))
        {
            LOG(Warning, "EndUpdateResource failed with error 0x{0:x}", (uint32)GetLastError());
            return true;
        }

        return false;
    }
}

IMPLEMENT_ENGINE_SETTINGS_GETTER(WindowsPlatformSettings, WindowsPlatform);

const Char* WindowsPlatformTools::GetDisplayName() const
{
    return TEXT("Windows");
}

const Char* WindowsPlatformTools::GetName() const
{
    return TEXT("Windows");
}

PlatformType WindowsPlatformTools::GetPlatform() const
{
    return PlatformType::Windows;
}

ArchitectureType WindowsPlatformTools::GetArchitecture() const
{
    return _arch;
}

bool WindowsPlatformTools::UseSystemDotnet() const
{
    return true;
}

bool WindowsPlatformTools::OnDeployBinaries(CookingData& data)
{
    const auto platformSettings = WindowsPlatformSettings::Get();

    Array<String> files;
    FileSystem::DirectoryGetFiles(files, data.NativeCodeOutputPath, TEXT("*.exe"), DirectorySearchOption::TopDirectoryOnly);
    if (files.HasItems())
    {
        // Apply executable icon
        TextureData iconData;
        if (!EditorUtilities::GetApplicationImage(platformSettings->OverrideIcon, iconData))
        {
            if (UpdateExeIcon(files[0], iconData))
            {
                data.Error(TEXT("Failed to change output executable file icon."));
                return true;
            }
        }

        // Apply executable version info
        if (UpdateExeVersion(files[0], data, platformSettings))
        {
            data.Error(TEXT("Failed to change output executable metadata."));
            return true;
        }

        // Rename app
        const String newName = EditorUtilities::GetOutputName();
        const StringView oldName = StringUtils::GetFileNameWithoutExtension(files[0]);
        if (newName != oldName)
        {
            if (FileSystem::MoveFile(data.NativeCodeOutputPath / newName + TEXT(".exe"), files[0], true))
            {
                data.Error(String::Format(TEXT("Failed to change output executable name from '{}' to '{}'."), oldName, newName));
                return true;
            }
        }
    }

    return false;
}

void WindowsPlatformTools::OnBuildStarted(CookingData& data)
{
    if (EnumHasAllFlags(data.Options, BuildOptions::NoCook))
        return;

    // Remove old executable
    Array<String> files;
    FileSystem::DirectoryGetFiles(files, data.NativeCodeOutputPath, TEXT("*.exe"), DirectorySearchOption::TopDirectoryOnly);
    for (auto& file : files)
        FileSystem::DeleteFile(file);
}

void WindowsPlatformTools::OnRun(CookingData& data, String& executableFile, String& commandLineFormat, String& workingDir)
{
    // Pick the first executable file
    Array<String> files;
    FileSystem::DirectoryGetFiles(files, data.NativeCodeOutputPath, TEXT("*.exe"), DirectorySearchOption::TopDirectoryOnly);
    if (files.HasItems())
    {
        executableFile = files[0];
    }
}

#endif
