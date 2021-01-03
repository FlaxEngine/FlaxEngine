// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "FontAsset.h"
#include "Font.h"
#include "FontManager.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/FontAssetUpgrader.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "IncludeFreeType.h"
#if USE_EDITOR
#include "Engine/Platform/FileSystem.h"
#endif

REGISTER_BINARY_ASSET(FontAsset, "FlaxEngine.FontAsset", ::New<FontAssetUpgrader>(), false);

FontAsset::FontAsset(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
    , _face(nullptr)
{
}

String FontAsset::GetFamilyName() const
{
    return _face ? String(_face->family_name) : String::Empty;
}

String FontAsset::GetStyleName() const
{
    return _face ? String(_face->style_name) : String::Empty;
}

Asset::LoadResult FontAsset::load()
{
    // Load font data
    auto chunk0 = GetChunk(0);
    if (chunk0 == nullptr || chunk0->IsMissing())
        return LoadResult::MissingDataChunk;
    _fontFile.Swap(chunk0->Data);

    // Create font face
    const FT_Error error = FT_New_Memory_Face(FontManager::GetLibrary(), _fontFile.Get(), static_cast<FT_Long>(_fontFile.Length()), 0, &_face);
    if (error)
    {
        _face = nullptr;
        LOG_FT_ERROR(error);
        return LoadResult::Failed;
    }

    return LoadResult::Ok;
}

void FontAsset::unload(bool isReloading)
{
    // Ensure to cleanup child font objects
    if (_fonts.HasItems())
    {
        LOG(Warning, "Font asset {0} is unloading but has {1} reaming font objects created", ToString(), _fonts.Count());
        for (auto font : _fonts)
        {
            font->_asset = nullptr;
            font->DeleteObject();
        }
        _fonts.Clear();
    }

    // Unload face
    if (_face)
    {
        FT_Done_Face(_face);
        _face = nullptr;
    }

    // Cleanup data
    _fontFile.Release();
}

AssetChunksFlag FontAsset::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}

Font* FontAsset::CreateFont(int32 size)
{
    PROFILE_CPU();

    if (WaitForLoaded())
        return nullptr;

    ScopeLock lock(Locker);

    // Check if font with that size has already been created
    for (auto font : _fonts)
    {
        if (font->GetAsset() == this && font->GetSize() == size)
            return font;
    }

    return New<Font>(this, size);
}

#if USE_EDITOR

bool FontAsset::Save(const StringView& path)
{
    // Validate state
    if (WaitForLoaded())
    {
        LOG(Error, "Asset loading failed. Cannot save it.");
        return true;
    }
    if (IsVirtual() && path.IsEmpty())
    {
        LOG(Error, "To save virtual asset asset you need to specify the target asset path location.");
        return true;
    }

    ScopeLock lock(Locker);

    AssetInitData data;
    data.SerializedVersion = SerializedVersion;
    data.CustomData.Copy(&_options);
    auto chunk0 = GetChunk(0);
    _fontFile.Swap(chunk0->Data);
    const bool saveResult = path.HasChars() ? SaveAsset(path, data) : SaveAsset(data, true);
    _fontFile.Swap(chunk0->Data);
    if (saveResult)
    {
        LOG(Error, "Cannot save '{0}'", ToString());
        return true;
    }

    return false;
}

#endif

void FontAsset::Invalidate()
{
    ScopeLock lock(Locker);

    for (auto font : _fonts)
    {
        font->Invalidate();
    }
}

bool FontAsset::init(AssetInitData& initData)
{
    // Validate
    if (initData.SerializedVersion != SerializedVersion)
    {
        LOG(Error, "Invalid serialized font asset version.");
        return true;
    }
    if (initData.CustomData.Length() != sizeof(_options))
    {
        LOG(Error, "Missing font asset header.");
        return true;
    }

    // Copy header
    Platform::MemoryCopy(&_options, initData.CustomData.Get(), sizeof(_options));

    return false;
}
