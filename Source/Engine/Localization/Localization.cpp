// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Localization.h"
#include "CultureInfo.h"
#include "LocalizationSettings.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Content/Content.h"
#include "Engine/Serialization/Serialization.h"

class LocalizationService : public EngineService
{
public:
    LocalizationService()
        : EngineService(TEXT("Localization"), -950)
    {
    }

    bool Init() override;
};

namespace
{
    CultureInfo CurrentCulture(0);
    CultureInfo CurrentLanguage(0);
    LocalizationService LocalizationServiceInstance;

IMPLEMENT_SETTINGS_GETTER(LocalizationSettings, Localization);

void LocalizationSettings::Apply()
{
}

void LocalizationSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(LocalizedStringTables);
}

}

bool LocalizationService::Init()
{
    CurrentLanguage = CurrentCulture = CultureInfo(Platform::GetUserLocaleName());
    return false;
}

Delegate<> Localization::LocalizationChanged;

const CultureInfo& Localization::GetCurrentCulture()
{
    return CurrentCulture;
}

void Localization::SetCurrentCulture(const CultureInfo& value)
{
    if (CurrentCulture == value)
        return;

    LOG(Info, "Changing current culture to: {0} ({1})", value.GetName(), value.GetLCID());
    CurrentCulture = value;
    LocalizationChanged();
}

const CultureInfo& Localization::GetCurrentLanguage()
{
    return CurrentLanguage;
}

void Localization::SetCurrentLanguage(const CultureInfo& value)
{
    if (CurrentLanguage == value)
        return;

    LOG(Info, "Changing current language to: {0} ({1})", value.GetName(), value.GetLCID());
    CurrentLanguage = value;
    LocalizationChanged();
}

void Localization::SetCurrentLanguageCulture(const CultureInfo& value)
{
    if (CurrentCulture == value && CurrentLanguage == value)
        return;

    LOG(Info, "Changing current language and culture to: {0} ({1})", value.GetName(), value.GetLCID());
    CurrentCulture = value;
    CurrentLanguage = value;
    LocalizationChanged();
}
