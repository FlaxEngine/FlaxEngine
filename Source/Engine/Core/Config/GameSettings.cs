// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    partial class GameSettings
    {
        internal const string PS4PlatformSettingsTypename = "FlaxEditor.Content.Settings.PS4PlatformSettings";
        internal const string XboxScarlettPlatformSettingsTypename = "FlaxEditor.Content.Settings.XboxScarlettPlatformSettings";

        /// <summary>
        /// The default application icon.
        /// </summary>
        [EditorOrder(30), EditorDisplay("General"), Tooltip("The default icon of the application.")]
        public Texture Icon;

        /// <summary>
        /// Reference to the first scene to load on a game startup.
        /// </summary>
        [EditorOrder(900), EditorDisplay("Startup"), Tooltip("Reference to the first scene to load on a game startup.")]
        public SceneReference FirstScene;

        /// <summary>
        /// True if skip showing splash screen image on the game startup.
        /// </summary>
        [EditorOrder(910), EditorDisplay("Startup", "No Splash Screen"), Tooltip("True if skip showing splash screen image on the game startup.")]
        public bool NoSplashScreen;

        /// <summary>
        /// Reference to the splash screen image to show on a game startup.
        /// </summary>
        [EditorOrder(920), EditorDisplay("Startup"), Tooltip("Reference to the splash screen image to show on a game startup.")]
        public Texture SplashScreen;

        /// <summary>
        /// Reference to <see cref="TimeSettings"/> asset.
        /// </summary>
        [EditorOrder(1010), EditorDisplay("Other Settings"), AssetReference(typeof(TimeSettings), true), Tooltip("Reference to Time Settings asset")]
        public JsonAsset Time;

        /// <summary>
        /// Reference to <see cref="AudioSettings"/> asset.
        /// </summary>
        [EditorOrder(1015), EditorDisplay("Other Settings"), AssetReference(typeof(AudioSettings), true), Tooltip("Reference to Audio Settings asset")]
        public JsonAsset Audio;

        /// <summary>
        /// Reference to <see cref="LayersAndTagsSettings"/> asset.
        /// </summary>
        [EditorOrder(1020), EditorDisplay("Other Settings"), AssetReference(typeof(LayersAndTagsSettings), true), Tooltip("Reference to Layers & Tags Settings asset")]
        public JsonAsset LayersAndTags;

        /// <summary>
        /// Reference to <see cref="PhysicsSettings"/> asset.
        /// </summary>
        [EditorOrder(1030), EditorDisplay("Other Settings"), AssetReference(typeof(PhysicsSettings), true), Tooltip("Reference to Physics Settings asset")]
        public JsonAsset Physics;

        /// <summary>
        /// Reference to <see cref="InputSettings"/> asset.
        /// </summary>
        [EditorOrder(1035), EditorDisplay("Other Settings"), AssetReference(typeof(InputSettings), true), Tooltip("Reference to Input Settings asset")]
        public JsonAsset Input;

        /// <summary>
        /// Reference to <see cref="GraphicsSettings"/> asset.
        /// </summary>
        [EditorOrder(1040), EditorDisplay("Other Settings"), AssetReference(typeof(GraphicsSettings), true), Tooltip("Reference to Graphics Settings asset")]
        public JsonAsset Graphics;

        /// <summary>
        /// Reference to <see cref="NavigationSettings"/> asset.
        /// </summary>
        [EditorOrder(1045), EditorDisplay("Other Settings"), AssetReference(typeof(NavigationSettings), true), Tooltip("Reference to Navigation Settings asset")]
        public JsonAsset Navigation;

        /// <summary>
        /// Reference to <see cref="BuildSettings"/> asset.
        /// </summary>
        [EditorOrder(1050), EditorDisplay("Other Settings"), AssetReference(typeof(BuildSettings), true), Tooltip("Reference to Build Settings asset")]
        public JsonAsset GameCooking;

        /// <summary>
        /// The custom settings to use with a game. Can be specified by the user to define game-specific options and be used by the external plugins (used as key-value pair).
        /// </summary>
        [EditorOrder(1100), EditorDisplay("Other Settings"), Tooltip("The custom settings to use with a game. Can be specified by the user to define game-specific options and be used by the external plugins (used as key-value pair).")]
        public Dictionary<string, JsonAsset> CustomSettings;

#if FLAX_EDITOR || PLATFORM_WINDOWS
        /// <summary>
        /// Reference to <see cref="WindowsPlatformSettings"/> asset. Used to apply configuration on Windows platform.
        /// </summary>
        [EditorOrder(2010), EditorDisplay("Platform Settings", "Windows"), AssetReference(typeof(WindowsPlatformSettings), true), Tooltip("Reference to Windows Platform Settings asset")]
        public JsonAsset WindowsPlatform;
#endif

#if FLAX_EDITOR || PLATFORM_UWP || PLATFORM_XBOX_ONE
        /// <summary>
        /// Reference to <see cref="UWPPlatformSettings"/> asset. Used to apply configuration on Universal Windows Platform.
        /// </summary>
        [EditorOrder(2020), EditorDisplay("Platform Settings", "Universal Windows Platform"), AssetReference(typeof(UWPPlatformSettings), true), Tooltip("Reference to Universal Windows Platform Settings asset")]
        public JsonAsset UWPPlatform;
#endif

#if FLAX_EDITOR || PLATFORM_LINUX
        /// <summary>
        /// Reference to <see cref="LinuxPlatformSettings"/> asset. Used to apply configuration on Linux platform.
        /// </summary>
        [EditorOrder(2030), EditorDisplay("Platform Settings", "Linux"), AssetReference(typeof(LinuxPlatformSettings), true), Tooltip("Reference to Linux Platform Settings asset")]
        public JsonAsset LinuxPlatform;
#endif

#if FLAX_EDITOR || PLATFORM_PS4
        /// <summary>
        /// Reference to PS4 Platform Settings asset. Used to apply configuration on PS4 platform.
        /// </summary>
        [EditorOrder(2040), EditorDisplay("Platform Settings", "PlayStation 4"), AssetReference(PS4PlatformSettingsTypename, true), Tooltip("Reference to PS4 Platform Settings asset")]
        public JsonAsset PS4Platform;
#endif

#if FLAX_EDITOR || PLATFORM_XBOX_SCARLETT
        /// <summary>
        /// Reference to Xbox Scarlett Platform Settings asset. Used to apply configuration on Xbox Scarlett platform.
        /// </summary>
        [EditorOrder(2050), EditorDisplay("Platform Settings", "Xbox Scarlett"), AssetReference(XboxScarlettPlatformSettingsTypename, true), Tooltip("Reference to Xbox Scarlett Platform Settings asset")]
        public JsonAsset XboxScarlettPlatform;
#endif

#if FLAX_EDITOR || PLATFORM_ANDROID
        /// <summary>
        /// Reference to <see cref="AndroidPlatformSettings"/> asset. Used to apply configuration on Android platform.
        /// </summary>
        [EditorOrder(2060), EditorDisplay("Platform Settings", "Android"), AssetReference(typeof(AndroidPlatformSettings), true), Tooltip("Reference to Android Platform Settings asset")]
        public JsonAsset AndroidPlatform;
#endif

        /// <summary>
        /// Gets the absolute path to the game settings asset file.
        /// </summary>
        public static string GameSettingsAssetPath
        {
            get { return StringUtils.CombinePaths(Globals.ProjectContentFolder, "GameSettings.json"); }
        }

        /// <summary>
        /// Loads the game settings asset.
        /// </summary>
        /// <returns>The loaded game settings.</returns>
        public static GameSettings Load()
        {
            var asset = FlaxEngine.Content.LoadAsync<JsonAsset>(GameSettingsAssetPath);
            if (asset && !asset.WaitForLoaded())
            {
                if (asset.CreateInstance() is GameSettings result)
                    return result;
            }
            return new GameSettings();
        }

        private static T LoadAsset<T>(JsonAsset asset) where T : new()
        {
            if (asset && !asset.WaitForLoaded())
            {
                if (asset.CreateInstance() is T result)
                    return result;
            }
            return new T();
        }

        private static SettingsBase LoadAsset(JsonAsset asset, string typename)
        {
            if (asset && !asset.WaitForLoaded() && asset.DataTypeName == typename)
            {
                if (asset.CreateInstance() is SettingsBase result)
                    return result;
            }
            return null;
        }

        /// <summary>
        /// Loads the settings of the given type.
        /// </summary>
        /// <remarks>
        /// Supports loading game settings, any sub settings container (e.g. <see cref="PhysicsSettings"/>) and custom settings (see <see cref="CustomSettings"/>).
        /// </remarks>
        /// <code>
        /// var time = GameSettings.Load&amp;ltTimeSettings&amp;gt;();
        /// </code>
        /// <typeparam name="T">The game settings type (e.g. <see cref="TimeSettings"/>).</typeparam>
        /// <returns>Loaded settings object or null if fails.</returns>
        public static T Load<T>() where T : SettingsBase
        {
            var gameSettings = Load();
            var type = typeof(T);

            if (type == typeof(GameSettings))
                return gameSettings as T;

            if (type == typeof(TimeSettings))
                return LoadAsset<TimeSettings>(gameSettings.Time) as T;
            if (type == typeof(LayersAndTagsSettings))
                return LoadAsset<LayersAndTagsSettings>(gameSettings.LayersAndTags) as T;
            if (type == typeof(PhysicsSettings))
                return LoadAsset<PhysicsSettings>(gameSettings.Physics) as T;
            if (type == typeof(GraphicsSettings))
                return LoadAsset<GraphicsSettings>(gameSettings.Graphics) as T;
            if (type == typeof(NavigationSettings))
                return LoadAsset<NavigationSettings>(gameSettings.Navigation) as T;
            if (type == typeof(BuildSettings))
                return LoadAsset<BuildSettings>(gameSettings.GameCooking) as T;
            if (type == typeof(InputSettings))
                return LoadAsset<InputSettings>(gameSettings.Input) as T;
            if (type == typeof(AudioSettings))
                return LoadAsset<AudioSettings>(gameSettings.Audio) as T;
#if FLAX_EDITOR || PLATFORM_WINDOWS
            if (type == typeof(WindowsPlatformSettings))
                return LoadAsset<WindowsPlatformSettings>(gameSettings.WindowsPlatform) as T;
#endif
#if FLAX_EDITOR || PLATFORM_UWP || PLATFORM_XBOX_ONE
            if (type == typeof(UWPPlatformSettings))
                return LoadAsset<UWPPlatformSettings>(gameSettings.UWPPlatform) as T;
#endif
#if FLAX_EDITOR || PLATFORM_LINUX
            if (type == typeof(LinuxPlatformSettings))
                return LoadAsset<LinuxPlatformSettings>(gameSettings.LinuxPlatform) as T;
#endif
#if FLAX_EDITOR || PLATFORM_PS4
            if (type.FullName == PS4PlatformSettingsTypename)
                return LoadAsset(gameSettings.PS4Platform, PS4PlatformSettingsTypename) as T;
#endif
#if FLAX_EDITOR || PLATFORM_XBOX_SCARLETT
            if (type.FullName == XboxScarlettPlatformSettingsTypename)
                return LoadAsset(gameSettings.XboxScarlettPlatform, XboxScarlettPlatformSettingsTypename) as T;
#endif
#if FLAX_EDITOR || PLATFORM_ANDROID
            if (type == typeof(AndroidPlatformSettings))
                return LoadAsset<AndroidPlatformSettings>(gameSettings.AndroidPlatform) as T;
#endif

            if (gameSettings.CustomSettings != null)
            {
                foreach (var e in gameSettings.CustomSettings)
                {
                    if (e.Value && !e.Value.WaitForLoaded() && e.Value.DataTypeName == type.FullName)
                    {
                        var custom = e.Value.CreateInstance();
                        if (custom is T result)
                            return result;
                    }
                }
            }

            return null;
        }

#if FLAX_EDITOR
        private static bool SaveAsset<T>(GameSettings gameSettings, ref JsonAsset asset, T obj) where T : SettingsBase
        {
            if (asset)
            {
                // Override settings
                return Editor.SaveJsonAsset(asset.Path, obj);
            }

            // Create new settings asset and link it to the game settings
            var path = StringUtils.CombinePaths(Globals.ProjectContentFolder, CustomEditors.CustomEditorsUtil.GetPropertyNameUI(typeof(T).Name) + ".json");
            if (Editor.SaveJsonAsset(path, obj))
                return true;
            asset = FlaxEngine.Content.LoadAsync<JsonAsset>(path);
            return Editor.SaveJsonAsset(GameSettingsAssetPath, gameSettings);
        }

        /// <summary>
        /// Saves the settings of the given type.
        /// </summary>
        /// <remarks>
        /// Supports saving game settings, any sub settings container (e.g. <see cref="PhysicsSettings"/>).
        /// </remarks>
        /// <code>
        /// var time = GameSettings.Load&amp;ltTimeSettings&amp;gt;();
        /// time.TimeScale = 0.5f;
        /// GameSettings.Save&amp;ltTimeSettings&amp;gt;(time);
        /// </code>
        /// <typeparam name="T">The game settings type (e.g. <see cref="TimeSettings"/>).</typeparam>
        /// <returns>True if failed otherwise false.</returns>
        public static bool Save<T>(T obj) where T : SettingsBase
        {
            var type = typeof(T);

            if (type == typeof(GameSettings))
            {
                return Editor.SaveJsonAsset(GameSettingsAssetPath, obj);
            }

            var gameSettings = Load();

            if (type == typeof(TimeSettings))
                return SaveAsset(gameSettings, ref gameSettings.Time, obj);
            if (type == typeof(LayersAndTagsSettings))
                return SaveAsset(gameSettings, ref gameSettings.LayersAndTags, obj);
            if (type == typeof(PhysicsSettings))
                return SaveAsset(gameSettings, ref gameSettings.Physics, obj);
            if (type == typeof(GraphicsSettings))
                return SaveAsset(gameSettings, ref gameSettings.Graphics, obj);
            if (type == typeof(NavigationSettings))
                return SaveAsset(gameSettings, ref gameSettings.Navigation, obj);
            if (type == typeof(BuildSettings))
                return SaveAsset(gameSettings, ref gameSettings.GameCooking, obj);
            if (type == typeof(InputSettings))
                return SaveAsset(gameSettings, ref gameSettings.Input, obj);
            if (type == typeof(WindowsPlatformSettings))
                return SaveAsset(gameSettings, ref gameSettings.WindowsPlatform, obj);
            if (type == typeof(UWPPlatformSettings))
                return SaveAsset(gameSettings, ref gameSettings.UWPPlatform, obj);
            if (type == typeof(LinuxPlatformSettings))
                return SaveAsset(gameSettings, ref gameSettings.LinuxPlatform, obj);
            if (type.FullName == PS4PlatformSettingsTypename)
                return SaveAsset(gameSettings, ref gameSettings.PS4Platform, obj);
            if (type.FullName == XboxScarlettPlatformSettingsTypename)
                return SaveAsset(gameSettings, ref gameSettings.XboxScarlettPlatform, obj);
            if (type == typeof(AndroidPlatformSettings))
                return SaveAsset(gameSettings, ref gameSettings.AndroidPlatform, obj);
            if (type == typeof(AudioSettings))
                return SaveAsset(gameSettings, ref gameSettings.Audio, obj);

            return true;
        }

        /// <summary>
        /// Sets the custom settings (or unsets if provided asset is null).
        /// </summary>
        /// <param name="key">The custom key (must be unique per context).</param>
        /// <param name="customSettingsAsset">The custom settings asset.</param>
        /// <returns>True if failed otherwise false.</returns>
        public static bool SetCustomSettings(string key, JsonAsset customSettingsAsset)
        {
            if (key == null)
                throw new ArgumentNullException(nameof(key));

            var gameSettings = Load();

            if (customSettingsAsset == null && gameSettings.CustomSettings != null)
            {
                gameSettings.CustomSettings.Remove(key);
            }
            else
            {
                if (gameSettings.CustomSettings == null)
                    gameSettings.CustomSettings = new Dictionary<string, JsonAsset>();
                gameSettings.CustomSettings[key] = customSettingsAsset;
            }

            return Editor.SaveJsonAsset(GameSettingsAssetPath, gameSettings);
        }

        /// <summary>
        /// Loads the current game settings asset and applies it to the engine runtime configuration.
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Apply();
#endif
    }
}
