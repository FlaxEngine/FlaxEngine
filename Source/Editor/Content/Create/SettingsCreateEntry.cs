// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Settings;
using FlaxEditor.Scripting;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content.Create
{
    /// <summary>
    /// Engine settings asset creating handler. Allows to specify type of the settings to create (e.g. <see cref="GameSettings"/>, <see cref="TimeSettings"/>, etc.).
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.Create.CreateFileEntry" />
    internal class SettingsCreateEntry : CreateFileEntry
    {
        /// <summary>
        /// Types of the settings assets that can be created.
        /// </summary>
        internal enum SettingsTypes
        {
            GameSettings,
            AudioSettings,
            TimeSettings,
            LayersAndTagsSettings,
            PhysicsSettings,
            GraphicsSettings,
            NavigationSettings,
            LocalizationSettings,
            BuildSettings,
            InputSettings,
            StreamingSettings,
            WindowsPlatformSettings,
            [EditorDisplay(null, "UWP Platform Settings")]
            UWPPlatformSettings,
            LinuxPlatformSettings,
            [EditorDisplay(null, "PS4 Platform Settings")]
            PS4PlatformSettings,
            XboxOnePlatformSettings,
            XboxScarlettPlatformSettings,
            AndroidPlatformSettings,
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
            typeof(StreamingSettings),
            typeof(WindowsPlatformSettings),
            typeof(UWPPlatformSettings),
            typeof(LinuxPlatformSettings),
            TypeUtils.GetManagedType(GameSettings.PS4PlatformSettingsTypename),
            TypeUtils.GetManagedType(GameSettings.XboxOnePlatformSettingsTypename),
            TypeUtils.GetManagedType(GameSettings.XboxScarlettPlatformSettingsTypename),
            typeof(AndroidPlatformSettings),
            TypeUtils.GetManagedType(GameSettings.SwitchPlatformSettingsTypename),
        };

        internal class Options
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
            if (Editor.SaveJsonAsset(ResultUrl, data))
                return true;

            // Automatic settings linking to game settings for easier usage
            var gameSettingsItem = Editor.Instance.ContentDatabase.Game.Content.Folder.FindChild(GameSettings.GameSettingsAssetPath) as JsonAssetItem;
            if (gameSettingsItem != null)
            {
                var gameSettingsWindow = Editor.Instance.Windows.FindEditor(gameSettingsItem) as JsonAssetWindow;
                if (gameSettingsWindow?.Instance is GameSettings)
                {
                    if (TrySet(gameSettingsWindow.Instance as GameSettings, ResultUrl, _options.Type))
                        gameSettingsWindow.MarkAsEdited();
                }
                else
                {
                    var gameSettingsAsset = FlaxEngine.Content.LoadAsync<JsonAsset>(gameSettingsItem.ID);
                    if (gameSettingsAsset && !gameSettingsAsset.WaitForLoaded())
                    {
                        if (gameSettingsAsset.CreateInstance() is GameSettings settings)
                        {
                            if (TrySet(settings, ResultUrl, _options.Type))
                            {
                                Editor.SaveJsonAsset(GameSettings.GameSettingsAssetPath, settings);
                            }
                        }
                    }
                }
            }

            return false;
        }

        private static bool TrySet(GameSettings instance, string resultUrl, SettingsTypes type)
        {
            var asset = FlaxEngine.Content.LoadAsync<JsonAsset>(resultUrl);
            if (instance != null && asset != null)
            {
                switch (type)
                {
                case SettingsTypes.AudioSettings:
                    if (instance.Audio != null)
                        return false;
                    instance.Audio = asset;
                    break;
                case SettingsTypes.TimeSettings:
                    if (instance.Time != null)
                        return false;
                    instance.Time = asset;
                    break;
                case SettingsTypes.LayersAndTagsSettings:
                    if (instance.LayersAndTags != null)
                        return false;
                    instance.LayersAndTags = asset;
                    break;
                case SettingsTypes.PhysicsSettings:
                    if (instance.Physics != null)
                        return false;
                    instance.Physics = asset;
                    break;
                case SettingsTypes.GraphicsSettings:
                    if (instance.Graphics != null)
                        return false;
                    instance.Graphics = asset;
                    break;
                case SettingsTypes.NavigationSettings:
                    if (instance.Navigation != null)
                        return false;
                    instance.Navigation = asset;
                    break;
                case SettingsTypes.LocalizationSettings:
                    if (instance.Localization != null)
                        return false;
                    instance.Localization = asset;
                    break;
                case SettingsTypes.BuildSettings:
                    if (instance.GameCooking != null)
                        return false;
                    instance.GameCooking = asset;
                    break;
                case SettingsTypes.InputSettings:
                    if (instance.Input != null)
                        return false;
                    instance.Input = asset;
                    break;
                case SettingsTypes.StreamingSettings:
                    if (instance.Streaming != null)
                        return false;
                    instance.Streaming = asset;
                    break;
                case SettingsTypes.WindowsPlatformSettings:
                    if (instance.WindowsPlatform != null)
                        return false;
                    instance.WindowsPlatform = asset;
                    break;
                case SettingsTypes.UWPPlatformSettings:
                    if (instance.UWPPlatform != null)
                        return false;
                    instance.UWPPlatform = asset;
                    break;
                case SettingsTypes.LinuxPlatformSettings:
                    if (instance.LinuxPlatform != null)
                        return false;
                    instance.LinuxPlatform = asset;
                    break;
                case SettingsTypes.PS4PlatformSettings:
                    if (instance.PS4Platform != null)
                        return false;
                    instance.PS4Platform = asset;
                    break;
                case SettingsTypes.XboxOnePlatformSettings:
                    if (instance.XboxOnePlatform != null)
                        return false;
                    instance.XboxOnePlatform = asset;
                    break;
                case SettingsTypes.XboxScarlettPlatformSettings:
                    if (instance.XboxScarlettPlatform != null)
                        return false;
                    instance.XboxScarlettPlatform = asset;
                    break;
                case SettingsTypes.AndroidPlatformSettings:
                    if (instance.AndroidPlatform != null)
                        return false;
                    instance.AndroidPlatform = asset;
                    break;
                case SettingsTypes.SwitchPlatformSettings:
                    if (instance.SwitchPlatform != null)
                        return false;
                    instance.SwitchPlatform = asset;
                    break;
                }
                return true;
            }
            return false;
        }
    }
}
