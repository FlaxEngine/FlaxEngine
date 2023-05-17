// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_IOS || PLATFORM_MAC || USE_EDITOR

#include "Engine/Core/Config/PlatformSettingsBase.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Scripting/SoftObjectReference.h"

class Texture;

/// <summary>
/// Apple platform settings.
/// </summary>
API_CLASS(abstract, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API ApplePlatformSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ApplePlatformSettings);

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

#if USE_EDITOR
    // Package code signing modes.
    API_ENUM() enum class CodeSigningMode
    {
        // No code signing.
        None,
        // Signing code with Provision File (.mobileprovision).
        WithProvisionFile,
    };

    /// <summary>
    /// App code signing mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2000), EditorDisplay(\"Code Signing\")")
    CodeSigningMode CodeSigning = CodeSigningMode::None;

    /// <summary>
    /// App code signing provision file path (absolute or relative to the project).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2050), EditorDisplay(\"Code Signing\")")
    String ProvisionFile;

    /// <summary>
    /// App code signing idenity name (from local Mac keychain). Use 'security find-identity -v -p codesigning' to list possible options.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2080), EditorDisplay(\"Code Signing\")")
    String SignIdenity;
#endif

public:
    // [SettingsBase]
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override
    {
        DESERIALIZE(AppIdentifier);
        DESERIALIZE(OverrideIcon);
#if USE_EDITOR
        DESERIALIZE(CodeSigning);
        DESERIALIZE(ProvisionFile);
        DESERIALIZE(SignIdenity);
#endif
    }
};

#endif
