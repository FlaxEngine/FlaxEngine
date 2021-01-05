// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/JsonTools.h"
#include "VirtualInput.h"

/// <summary>
/// Input settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API InputSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(InputSettings);
public:

    /// <summary>
    /// Maps a discrete button or key press events to a "friendly name" that will later be bound to event-driven behavior. The end effect is that pressing (and/or releasing) a key, mouse button, or keypad button.
    /// </summary>
    Array<ActionConfig> ActionMappings;

    /// <summary>
    /// Maps keyboard, controller, or mouse inputs to a "friendly name" that will later be bound to continuous game behavior, such as movement. The inputs mapped in AxisMappings are continuously polled, even if they are just reporting that their input value.
    /// </summary>
    Array<AxisConfig> AxisMappings;

public:

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static InputSettings* Get();

    // [SettingsBase]
    void Apply() override;

    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        const auto actionMappings = stream.FindMember("ActionMappings");
        if (actionMappings != stream.MemberEnd())
        {
            auto& actionMappingsArray = actionMappings->value;
            ASSERT(actionMappingsArray.IsArray());
            ActionMappings.Resize(actionMappingsArray.Size(), false);

            for (uint32 i = 0; i < actionMappingsArray.Size(); i++)
            {
                auto& v = actionMappingsArray[i];
                if (!v.IsObject())
                    continue;

                ActionConfig& config = ActionMappings[i];

                config.Name = JsonTools::GetString(v, "Name");
                config.Mode = JsonTools::GetEnum(v, "Mode", InputActionMode::Pressing);
                config.Key = JsonTools::GetEnum(v, "Key", KeyboardKeys::None);
                config.MouseButton = JsonTools::GetEnum(v, "MouseButton", MouseButton::None);
                config.GamepadButton = JsonTools::GetEnum(v, "GamepadButton", GamepadButton::None);
                config.Gamepad = JsonTools::GetEnum(v, "Gamepad", InputGamepadIndex::All);
            }
        }

        const auto axisMappings = stream.FindMember("AxisMappings");
        if (axisMappings != stream.MemberEnd())
        {
            auto& axisMappingsArray = axisMappings->value;
            ASSERT(axisMappingsArray.IsArray());
            AxisMappings.Resize(axisMappingsArray.Size(), false);

            for (uint32 i = 0; i < axisMappingsArray.Size(); i++)
            {
                auto& v = axisMappingsArray[i];
                if (!v.IsObject())
                    continue;

                AxisConfig& config = AxisMappings[i];

                config.Name = JsonTools::GetString(v, "Name");
                config.Axis = JsonTools::GetEnum(v, "Axis", InputAxisType::MouseX);
                config.Gamepad = JsonTools::GetEnum(v, "Gamepad", InputGamepadIndex::All);
                config.PositiveButton = JsonTools::GetEnum(v, "PositiveButton", KeyboardKeys::None);
                config.NegativeButton = JsonTools::GetEnum(v, "NegativeButton", KeyboardKeys::None);
                config.DeadZone = JsonTools::GetFloat(v, "DeadZone", 0.1f);
                config.Sensitivity = JsonTools::GetFloat(v, "Sensitivity", 0.4f);
                config.Gravity = JsonTools::GetFloat(v, "Gravity", 1.0f);
                config.Scale = JsonTools::GetFloat(v, "Scale", 1.0f);
                config.Snap = JsonTools::GetBool(v, "Snap", false);
            }
        }
    }
};
