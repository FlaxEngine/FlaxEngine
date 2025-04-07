// Copyright (c) Wojciech Figat. All rights reserved.

#include "FontAsset.h"
#include "Font.h"
#include "FontManager.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/FontAssetUpgrader.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"
#include "IncludeFreeType.h"
#if USE_EDITOR
#include "Engine/Platform/FileSystem.h"
#endif

REGISTER_BINARY_ASSET_WITH_UPGRADER(FontAsset, "FlaxEngine.FontAsset", FontAssetUpgrader, true);

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
    return Init() ? LoadResult::Failed : LoadResult::Ok;
}

void FontAsset::unload(bool isReloading)
{
    // Ensure to cleanup child font objects
    if (_fonts.HasItems())
    {
        LOG(Warning, "Font asset {0} is unloading but has {1} remaining font objects created", ToString(), _fonts.Count());
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
    _virtualBold = nullptr;
    _virtualItalic = nullptr;
}

AssetChunksFlag FontAsset::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}

bool FontAsset::Init()
{
    const FT_Error error = FT_New_Memory_Face(FontManager::GetLibrary(), _fontFile.Get(), static_cast<FT_Long>(_fontFile.Length()), 0, &_face);
    if (error)
    {
        _face = nullptr;
        LOG_FT_ERROR(error);
    }
    return error;
}

FontFlags FontAsset::GetStyle() const
{
    FontFlags flags = FontFlags::None;
    if ((_face->style_flags & FT_STYLE_FLAG_ITALIC) != 0)
        flags |= FontFlags::Italic;
    if ((_face->style_flags & FT_STYLE_FLAG_BOLD) != 0)
        flags |= FontFlags::Bold;
    return flags;
}

void FontAsset::SetOptions(const FontOptions& value)
{
    _options = value;
}

Font* FontAsset::CreateFont(float size)
{
    PROFILE_CPU();

    if (WaitForLoaded())
        return nullptr;

    ScopeLock lock(Locker);
    if (_face == nullptr)
        return nullptr;

    // Check if font with that size has already been created
    for (auto font : _fonts)
    {
        if (font->GetAsset() == this && font->GetSize() == size)
            return font;
    }

    return New<Font>(this, size);
}

FontAsset* FontAsset::GetBold()
{
    ScopeLock lock(Locker);
    if (EnumHasAnyFlags(_options.Flags, FontFlags::Bold))
        return this;
    if (!_virtualBold)
    {
        _virtualBold = Content::CreateVirtualAsset<FontAsset>();
        _virtualBold->Init(_fontFile);
        auto options = _options;
        options.Flags |= FontFlags::Bold;
        _virtualBold->SetOptions(options);
    }
    return _virtualBold;
}

FontAsset* FontAsset::GetItalic()
{
    ScopeLock lock(Locker);
    if (EnumHasAnyFlags(_options.Flags, FontFlags::Italic))
        return this;
    if (!_virtualItalic)
    {
        _virtualItalic = Content::CreateVirtualAsset<FontAsset>();
        _virtualItalic->Init(_fontFile);
        auto options = _options;
        options.Flags |= FontFlags::Italic;
        _virtualItalic->SetOptions(options);
    }
    return _virtualItalic;
}

bool FontAsset::Init(const BytesContainer& fontFile)
{
    ScopeLock lock(Locker);
    unload(true);
    _fontFile.Copy(fontFile);
    return Init();
}

#if USE_EDITOR

bool FontAsset::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
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

bool FontAsset::ContainsChar(Char c) const
{
    return FT_Get_Char_Index(_face, c) > 0;
}

void FontAsset::Invalidate()
{
    ScopeLock lock(Locker);
    for (auto font : _fonts)
        font->Invalidate();
}

uint64 FontAsset::GetMemoryUsage() const
{
    Locker.Lock();
    uint64 result = BinaryAsset::GetMemoryUsage();
    result += sizeof(FontAsset) - sizeof(BinaryAsset);
    result += sizeof(FT_FaceRec);
    result += _fontFile.Length();
    for (auto font : _fonts)
        result += sizeof(Font);
    Locker.Unlock();
    return result;
}

bool FontAsset::init(AssetInitData& initData)
{
    if (IsVirtual())
        return false;
    if (initData.CustomData.Length() != sizeof(_options))
    {
        LOG(Error, "Missing font asset header.");
        return true;
    }

    // Copy header
    Platform::MemoryCopy(&_options, initData.CustomData.Get(), sizeof(_options));

    return false;
}
