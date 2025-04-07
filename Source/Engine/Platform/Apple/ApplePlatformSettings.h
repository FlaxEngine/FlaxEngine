// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_IOS || PLATFORM_MAC || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Scripting/SoftObjectReference.h"

class Texture;

/// <summary>
/// Apple platform settings.
/// </summary>
API_CLASS(Abstract, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API ApplePlatformSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ApplePlatformSettings);
    API_AUTO_SERIALIZATION();

    /// <summary>
    /// The app identifier (reversed DNS, eg. com.company.product). Custom tokens: ${PROJECT_NAME}, ${COMPANY_NAME}.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"General\")")
    String AppIdentifier = TEXT("com.${COMPANY_NAME}.${PROJECT_NAME}");

    /// <summary>
    /// Custom icon texture to use for the application (overrides the default one).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1000), EditorDisplay(\"Other\")")
    SoftObjectReference<Texture> OverrideIcon;
};

#endif
