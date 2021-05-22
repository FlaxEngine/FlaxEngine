// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Settings;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Content.Create
{
    /// <summary>
    /// Engine settings asset creating handler. Allows to specify type of the settings to create (e.g. <see cref="GameSettings"/>, <see cref="TimeSettings"/>, etc.).
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.Create.CreateFileEntry" />
    public class SettingsCreateEntry : CreateFileEntry
    {
        /// <summary>
        /// Types of the settings assets that can be created.
        /// </summary>
        public enum SettingsTypes
        {
            /// <summary>
            /// The game settings.
            /// </summary>
            GameSettings,

            /// <summary>
            /// The audio settings.
            /// </summary>
            AudioSettings,

            /// <summary>
            /// The time settings.
            /// </summary>
            TimeSettings,

            /// <summary>
            /// The layers and tags settings.
            /// </summary>
            LayersAndTagsSettings,

            /// <summary>
            /// The physics settings.
            /// </summary>
            PhysicsSettings,

            /// <summary>
            /// The graphics settings.
            /// </summary>
            GraphicsSettings,

            /// <summary>
            /// The navigation settings.
            /// </summary>
            NavigationSettings,

            /// <summary>
            /// The localization settings.
            /// </summary>
            LocalizationSettings,

            /// <summary>
            /// The build settings.
            /// </summary>
            BuildSettings,

            /// <summary>
            /// The input settings.
            /// </summary>
            InputSettings,

            /// <summary>
            /// The Windows settings.
            /// </summary>
            WindowsPlatformSettings,

            /// <summary>
            /// The UWP settings.
            /// </summary>
            UWPPlatformSettings,

            /// <summary>
            /// The Linux settings.
            /// </summary>
            LinuxPlatformSettings,

            /// <summary>
            /// The PS4 settings
            /// </summary>
            PS4PlatformSettings,

            /// <summary>
            /// The Xbox Scarlett settings
            /// </summary>
            XboxScarlettPlatformSettings,

            /// <summary>
            /// The Android settings
            /// </summary>
            AndroidPlatformSettings,

            /// <summary>
            /// The Switch settings
            /// </summary>
            SwitchPlatformSettings,
        }

        private static readonly Type[] _types =
        {
            typeof(GameSettings),
            typeof(AudioSettings),
            typeof(TimeSettings),
            typeof(LayersAndTagsSettings),
            typeof(PhysicsSettings),
            typeof(GraphicsSettings),
            typeof(NavigationSettings),
            typeof(LocalizationSettings),
            typeof(BuildSettings),
            typeof(InputSettings),
            typeof(WindowsPlatformSettings),
            typeof(UWPPlatformSettings),
            typeof(LinuxPlatformSettings),
            TypeUtils.GetManagedType(GameSettings.PS4PlatformSettingsTypename),
            TypeUtils.GetManagedType(GameSettings.XboxScarlettPlatformSettingsTypename),
            typeof(AndroidPlatformSettings),
            TypeUtils.GetManagedType(GameSettings.SwitchPlatformSettingsTypename),
        };

        /// <summary>
        /// The create options.
        /// </summary>
        public class Options
        {
            /// <summary>
            /// The type.
            /// </summary>
            [Tooltip("Type of the settings asset to create")]
            public SettingsTypes Type = SettingsTypes.GameSettings;
        }

        private readonly Options _options = new Options();

        /// <inheritdoc />
        public override object Settings => _options;

        /// <summary>
        /// Initializes a new instance of the <see cref="SettingsCreateEntry"/> class.
        /// </summary>
        /// <param name="resultUrl">The result file url.</param>
        public SettingsCreateEntry(string resultUrl)
        : base("Settings", resultUrl)
        {
        }

        /// <inheritdoc />
        public override bool Create()
        {
            // Create settings asset object and serialize it to pure json asset
            var type = _types[(int)_options.Type];
            if (type == null)
            {
                MessageBox.Show("Cannot create " + _options.Type + " settings. Platform not supported.");
                return true;
            }
            var data = Activator.CreateInstance(type);
            return Editor.SaveJsonAsset(ResultUrl, data);
        }
    }
}
