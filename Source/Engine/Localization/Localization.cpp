// Copyright (c) Wojciech Figat. All rights reserved.

#include "Localization.h"
#include "CultureInfo.h"
#include "LocalizedString.h"
#include "LocalizationSettings.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Content/Content.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/Serialization.h"
#include <locale>

class LocalizationService : public EngineService
{
public:
    CultureInfo CurrentCulture;
    CultureInfo CurrentLanguage;
    Array<AssetReference<LocalizedStringTable>> LocalizedStringTables;
    Array<AssetReference<LocalizedStringTable>> FallbackStringTables;

    LocalizationService()
        : EngineService(TEXT("Localization"), -500)
        , CurrentCulture(0)
        , CurrentLanguage(0)
    {
    }

    void OnLocalizationChanged();

    const String& Get(const String& id, int32 index, const String& fallback) const
    {
        if (id.IsEmpty())
            return fallback;

#define TRY_RETURN(messages) \
        if (messages && messages->Count() > index) \
        { \
            const String& result = messages->At(index); \
            if (result.HasChars()) \
                return result; \
        }

        // Try current tables
        for (auto& e : LocalizedStringTables)
        {
            const auto table = e.Get();
            const auto messages = table ? table->Entries.TryGet(id) : nullptr;
            TRY_RETURN(messages);
        }

        // Try fallback tables for current tables
        for (auto& e : LocalizedStringTables)
        {
            const auto table = e.Get();
            const auto fallbackTable = table ? table->FallbackTable.Get() : nullptr;
            const auto messages = fallbackTable ? fallbackTable->Entries.TryGet(id) : nullptr;
            TRY_RETURN(messages);
        }

        // Try fallback language tables
        for (auto& e : FallbackStringTables)
        {
            const auto table = e.Get();
            const auto messages = table ? table->Entries.TryGet(id) : nullptr;
            TRY_RETURN(messages);
        }

        return fallback;
    }

    bool Init() override;
};

namespace
{
    LocalizationService Instance;
}

IMPLEMENT_ENGINE_SETTINGS_GETTER(LocalizationSettings, Localization);

void LocalizationSettings::Apply()
{
    Instance.OnLocalizationChanged();
}

#if USE_EDITOR

void LocalizationSettings::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(LocalizationSettings);
    SERIALIZE(LocalizedStringTables);
    SERIALIZE(DefaultFallbackLanguage);
}

#endif

void LocalizationSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(LocalizedStringTables);
    DESERIALIZE(DefaultFallbackLanguage);
}

LocalizedString::LocalizedString(const LocalizedString& other)
    : Id(other.Id)
    , Value(other.Value)
{
}

LocalizedString::LocalizedString(LocalizedString&& other) noexcept
    : Id(MoveTemp(other.Id))
    , Value(MoveTemp(other.Value))
{
}

LocalizedString::LocalizedString(const StringView& value)
    : Value(value)
{
}

LocalizedString::LocalizedString(String&& value) noexcept
    : Value(MoveTemp(value))
{
}

LocalizedString& LocalizedString::operator=(const LocalizedString& other)
{
    if (this != &other)
    {
        Id = other.Id;
        Value = other.Value;
    }
    return *this;
}

LocalizedString& LocalizedString::operator=(LocalizedString&& other) noexcept
{
    if (this != &other)
    {
        Id = MoveTemp(other.Id);
        Value = MoveTemp(other.Value);
    }
    return *this;
}

LocalizedString& LocalizedString::operator=(const StringView& value)
{
    Id.Clear();
    Value = value;
    return *this;
}

LocalizedString& LocalizedString::operator=(String&& value) noexcept
{
    Id.Clear();
    Value = MoveTemp(value);
    return *this;
}

String LocalizedString::ToString() const
{
    return Localization::GetString(Id, Value);
}

String LocalizedString::ToStringPlural(int32 n) const
{
    return Localization::GetPluralString(Id, n, Value);
}

void LocalizationService::OnLocalizationChanged()
{
    PROFILE_CPU();

    Instance.LocalizedStringTables.Clear();
    Instance.FallbackStringTables.Clear();
    const StringView en(TEXT("en"));

    // Collect all localization tables into mapping locale -> tables
    auto& settings = *LocalizationSettings::Get();
    Dictionary<String, Array<AssetReference<LocalizedStringTable>, InlinedAllocation<32>>> tables;
    for (auto& e : settings.LocalizedStringTables)
    {
        auto table = e.Get();
        if (table && !table->WaitForLoaded())
        {
            tables[table->Locale].Add(table);
        }
    }

    // Pick localization tables for a current language
    auto* table = tables.TryGet(Instance.CurrentLanguage.GetName());
    if (!table)
    {
        // Try using parent culture (eg. en if en-GB is missing)
        const CultureInfo parentLanguage(Instance.CurrentLanguage.GetParentLCID());
        if (parentLanguage.GetName().HasChars())
            table = tables.TryGet(parentLanguage.GetName());
        if (!table)
        {
            // Fallback to Default
            table = tables.TryGet(settings.DefaultFallbackLanguage);
            if (!table)
            {
                // Fallback to English
                table = tables.TryGet(en);
            }
        }
    }

    // Apply localization table
    if (table)
    {
        String locale;
        for (auto& e : tables)
        {
            if (&e.Value == table)
            {
                locale = e.Key;
                break;
            }
        }
        LOG(Info, "Using localization for {0}", locale);
        Instance.LocalizedStringTables.Add(table->Get(), table->Count());
        if (locale != settings.DefaultFallbackLanguage || locale != en)
        {
            // Cache fallback language tables to support additional text resolving in case of missing entries in the current language
            table = tables.TryGet(settings.DefaultFallbackLanguage);
            if (!table)
                table = tables.TryGet(en);
            if (table)
            {
                Instance.FallbackStringTables.Add(table->Get(), table->Count());
            }
        }
    }

#if PLATFORM_ANDROID
    // Android doesn't support locales in the native C library (https://issuetracker.google.com/issues/36974962)
#else
    // Change C++ locale (eg. used by fmt lib for values formatting)
    {
        char localeName[100];
        const auto& currentCulture = Instance.CurrentCulture.GetName();
        if (currentCulture.IsEmpty())
        {
            localeName[0] = 0;
        }
        else
        {
            StringUtils::ConvertUTF162ANSI(currentCulture.GetText(), localeName, currentCulture.Length());
            for (int32 i = 0; i < currentCulture.Length(); i++)
                if (localeName[i] == '-')
                    localeName[i] = '_';
            localeName[currentCulture.Length() + 0] = '.';
            localeName[currentCulture.Length() + 1] = 'U';
            localeName[currentCulture.Length() + 2] = 'T';
            localeName[currentCulture.Length() + 3] = 'F';
            localeName[currentCulture.Length() + 4] = '-';
            localeName[currentCulture.Length() + 5] = '8';
            localeName[currentCulture.Length() + 6] = 0;
        }
        try
        {
            std::locale::global(std::locale(localeName));
        }
        catch (std::runtime_error const&)
        {
        }
        catch (...)
        {
        }
    }
#endif

    // Send event
    Localization::LocalizationChanged();
}

bool LocalizationService::Init()
{
    // Use system language as default
    CurrentLanguage = CurrentCulture = CultureInfo(Platform::GetUserLocaleName());

    // Setup localization
    Instance.OnLocalizationChanged();

    return false;
}

Delegate<> Localization::LocalizationChanged;

const CultureInfo& Localization::GetCurrentCulture()
{
    return Instance.CurrentCulture;
}

void Localization::SetCurrentCulture(const CultureInfo& value)
{
    if (Instance.CurrentCulture == value)
        return;

    LOG(Info, "Changing current culture to: {0} ({1})", value.GetName(), value.GetLCID());
    Instance.CurrentCulture = value;
    Instance.OnLocalizationChanged();
}

const CultureInfo& Localization::GetCurrentLanguage()
{
    return Instance.CurrentLanguage;
}

void Localization::SetCurrentLanguage(const CultureInfo& value)
{
    if (Instance.CurrentLanguage == value)
        return;

    LOG(Info, "Changing current language to: {0} ({1})", value.GetName(), value.GetLCID());
    Instance.CurrentLanguage = value;
    Instance.OnLocalizationChanged();
}

void Localization::SetCurrentLanguageCulture(const CultureInfo& value)
{
    if (Instance.CurrentCulture == value && Instance.CurrentLanguage == value)
        return;

    LOG(Info, "Changing current language and culture to: {0} ({1})", value.GetName(), value.GetLCID());
    Instance.CurrentCulture = value;
    Instance.CurrentLanguage = value;
    Instance.OnLocalizationChanged();
}

String Localization::GetString(const String& id, const String& fallback)
{
    return Instance.Get(id, 0, fallback);
}

String Localization::GetPluralString(const String& id, int32 n, const String& fallback)
{
    CHECK_RETURN(n >= 1, String::Format(fallback.GetText(), n));
    const String& format = Instance.Get(id, n - 1, fallback);
    return String::Format(format.GetText(), n);
}
