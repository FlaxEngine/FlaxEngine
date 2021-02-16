// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    partial class InputSettings
    {
        /// <summary>
        /// Maps a discrete button or key press events to a "friendly name" that will later be bound to event-driven behavior. The end effect is that pressing (and/or releasing) a key, mouse button, or keypad button.
        /// </summary>
        /// <seealso cref="Input.ActionMappings"/>
        [Collection(Spacing = 10)]
        [EditorOrder(100), EditorDisplay("Input Map"), Tooltip("Maps a discrete button or key press events to a \"friendly name\" that will later be bound to event-driven behavior. The end effect is that pressing (and/or releasing) a key, mouse button, or keypad button.")]
        public ActionConfig[] ActionMappings;

        /// <summary>
        /// Maps keyboard, controller, or mouse inputs to a "friendly name" that will later be bound to continuous game behavior, such as movement. The inputs mapped in AxisMappings are continuously polled, even if they are just reporting that their input value.
        /// </summary>
        /// <seealso cref="Input.AxisMappings"/>
        [Collection(Spacing = 10)]
        [EditorOrder(200), EditorDisplay("Input Map"), Tooltip("Maps keyboard, controller, or mouse inputs to a \"friendly name\" that will later be bound to continuous game behavior, such as movement. The inputs mapped in AxisMappings are continuously polled, even if they are just reporting that their input value.")]
        public AxisConfig[] AxisMappings;
    }
}
