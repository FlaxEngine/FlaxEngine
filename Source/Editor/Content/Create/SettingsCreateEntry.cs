// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Reflection;
using FlaxEditor.Content.Settings;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

#pragma warning disable CS0649

namespace FlaxEditor.Content.Create
{
    /// <summary>
    /// Engine settings asset creating handler. Allows to specify type of the settings to create (e.g. <see cref="GameSettings"/>, <see cref="TimeSettings"/>, etc.).
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.Create.CreateFileEntry" />
    internal class SettingsCreateEntry : CreateFileEntry
    {
        public override bool CanBeCreated => _options.Type != null;

        internal class Options
        {
            [Tooltip("The settings type.")]
            [TypeReference(checkMethod: nameof(IsValid))]
            public Type Type;

            [HideInEditor, NoSerialize]
            public static readonly List<Type> Types = new List<Type>();

            private static bool IsValid(Type type)
            {
                return Types.Contains(type);
            }
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
            // Find types for settings
            Options.Types.Clear();
            foreach (var proxy in Editor.Instance.ContentDatabase.Proxy)
            {
                if (proxy is SettingsProxy settingsProxy && settingsProxy.Type != null)
                    Options.Types.Add(settingsProxy.Type);
            }
        }

        /// <inheritdoc />
        public override bool Create()
        {
            // Create settings asset object and serialize it to pure json asset
            var type = _options.Type;
            if (type == null)
                return true;
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
                    if (TrySet(gameSettingsWindow.Instance as GameSettings, ResultUrl, type))
                        gameSettingsWindow.MarkAsEdited();
                }
                else
                {
                    var gameSettingsAsset = FlaxEngine.Content.LoadAsync<JsonAsset>(gameSettingsItem.ID);
                    if (gameSettingsAsset && !gameSettingsAsset.WaitForLoaded())
                    {
                        if (gameSettingsAsset.CreateInstance() is GameSettings settings)
                        {
                            if (TrySet(settings, ResultUrl, type))
                                Editor.SaveJsonAsset(GameSettings.GameSettingsAssetPath, settings);
                        }
                    }
                }
            }

            return false;
        }

        private static bool TrySet(GameSettings instance, string resultUrl, Type type)
        {
            var asset = FlaxEngine.Content.LoadAsync<JsonAsset>(resultUrl);
            if (instance != null && asset != null)
            {
                // Try set Game Settings field
                var fields = typeof(GameSettings).GetFields();
                foreach (var field in fields)
                {
                    if (field.FieldType == typeof(JsonAsset))
                    {
                        var attribute = field.GetCustomAttribute<AssetReferenceAttribute>();
                        if (attribute != null && attribute.TypeName == type.FullName)
                        {
                            var value = field.GetValue(instance);
                            if (value != null)
                                return false;
                            field.SetValue(instance, asset);
                            return true;
                        }
                    }
                }

                // Try with custom settings
                foreach (var proxy in Editor.Instance.ContentDatabase.Proxy)
                {
                    if (proxy is CustomSettingsProxy settingsProxy && settingsProxy.Type == type)
                    {
                        if (instance.CustomSettings != null && instance.CustomSettings.ContainsKey(settingsProxy.CustomName))
                            return false;
                        if (instance.CustomSettings == null)
                            instance.CustomSettings = new Dictionary<string, JsonAsset>();
                        instance.CustomSettings.Add(settingsProxy.CustomName, asset);
                        return true;
                    }
                }
            }
            return false;
        }
    }
}
