// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using FlaxEditor.Modules;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;

namespace FlaxEditor.Options
{
    /// <summary>
    /// Editor options management module.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class OptionsModule : EditorModule
    {
        /// <summary>
        /// The current editor options. Don't modify the values directly (local session state lifetime), use <see cref="Apply"/>.
        /// </summary>
        public EditorOptions Options = new EditorOptions();

        /// <summary>
        /// Occurs when editor options get changed (reloaded or applied).
        /// </summary>
        public event Action<EditorOptions> OptionsChanged;

        /// <summary>
        /// Occurs when editor options get changed (reloaded or applied).
        /// </summary>
        public event Action CustomSettingsChanged;

        /// <summary>
        /// The custom settings factory delegate. It should return the default settings object for a given options content.
        /// </summary>
        /// <returns>The custom settings object.</returns>
        public delegate object CreateCustomSettingsDelegate();

        private readonly string _optionsFilePath;
        private readonly Dictionary<string, CreateCustomSettingsDelegate> _customSettings = new Dictionary<string, CreateCustomSettingsDelegate>();

        /// <summary>
        /// Gets the custom settings factories. Each entry defines the custom settings type identified by the given key name. The value is a factory function that returns the default options for a given type.
        /// </summary>
        public IReadOnlyDictionary<string, CreateCustomSettingsDelegate> CustomSettings => _customSettings;

        internal OptionsModule(Editor editor)
        : base(editor)
        {
            // Always load options before the other modules setup
            InitOrder = -1000000;

            _optionsFilePath = Path.Combine(Editor.LocalCachePath, "EditorOptions.json");
        }

        /// <summary>
        /// Adds the custom settings factory.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="factory">The factory function.</param>
        public void AddCustomSettings(string name, CreateCustomSettingsDelegate factory)
        {
            if (_customSettings.ContainsKey(name))
                throw new ArgumentException(string.Format("Custom settings \'{0}\' already added.", name), nameof(name));

            Editor.Log(string.Format("Add custom editor settings \'{0}\'", name));
            _customSettings.Add(name, factory);
            if (!Options.CustomSettings.ContainsKey(name))
            {
                Options.CustomSettings.Add(name, JsonSerializer.Serialize(factory(), typeof(object)));
            }
            CustomSettingsChanged?.Invoke();
        }

        /// <summary>
        /// Removes the custom settings factory.
        /// </summary>
        /// <param name="name">The name.</param>
        public void RemoveCustomSettings(string name)
        {
            if (!_customSettings.ContainsKey(name))
                throw new ArgumentException(string.Format("Custom settings \'{0}\' has not been added.", name), nameof(name));

            Editor.Log(string.Format("Remove custom editor settings \'{0}\'", name));
            _customSettings.Remove(name);
            CustomSettingsChanged?.Invoke();
        }

        /// <summary>
        /// Loads the settings from the file.
        /// </summary>
        public void Load()
        {
            Editor.Log("Loading editor options");

            try
            {
                // Load asset
                var asset = FlaxEngine.Content.LoadAsync<JsonAsset>(_optionsFilePath);
                if (asset == null)
                {
                    Editor.LogWarning("Missing or invalid editor settings");
                    return;
                }
                if (asset.WaitForLoaded())
                {
                    Editor.LogWarning("Failed to load editor settings");
                    return;
                }

                // Deserialize data
                var assetObj = asset.CreateInstance();
                if (assetObj is EditorOptions options)
                {
                    // Add missing custom options
                    foreach (var e in _customSettings)
                    {
                        if (!options.CustomSettings.ContainsKey(e.Key))
                            options.CustomSettings.Add(e.Key, JsonSerializer.Serialize(e.Value()));
                    }

                    Options = options;
                    OnOptionsChanged();
                    Platform.CustomDpiScale = Options.Interface.InterfaceScale;
                }
                else
                {
                    Editor.LogWarning("Failed to deserialize editor settings");
                }
            }
            catch (Exception ex)
            {
                Editor.LogError("Failed to load editor options.");
                Editor.LogWarning(ex);
            }
        }

        /// <summary>
        /// Applies the specified options and updates the dependant services.
        /// </summary>
        /// <param name="options">The new options.</param>
        public void Apply(EditorOptions options)
        {
            Options = options;
            OnOptionsChanged();
            Save();
        }

        private void Save()
        {
            // Update file
            Editor.SaveJsonAsset(_optionsFilePath, Options);

            // Special case for editor analytics
            var editorAnalyticsTrackingFile = Path.Combine(Editor.LocalCachePath, "noTracking");
            if (Options.General.EnableEditorAnalytics)
            {
                if (!File.Exists(editorAnalyticsTrackingFile))
                {
                    File.WriteAllText(editorAnalyticsTrackingFile, "Don't track me, please.");
                }
            }
            else
            {
                if (File.Exists(editorAnalyticsTrackingFile))
                {
                    File.Delete(editorAnalyticsTrackingFile);
                }
            }
        }

        private void OnOptionsChanged()
        {
            Editor.Log("Editor options changed!");

            // Sync C++ backend options
            Editor.InternalOptions internalOptions;
            internalOptions.AutoReloadScriptsOnMainWindowFocus = (byte)(Options.General.AutoReloadScriptsOnMainWindowFocus ? 1 : 0);
            internalOptions.ForceScriptCompilationOnStartup = (byte)(Options.General.ForceScriptCompilationOnStartup ? 1 : 0);
            internalOptions.AutoRebuildCSG = (byte)(Options.General.AutoRebuildCSG ? 1 : 0);
            internalOptions.AutoRebuildCSGTimeoutMs = Options.General.AutoRebuildCSGTimeoutMs;
            internalOptions.AutoRebuildNavMesh = (byte)(Options.General.AutoRebuildNavMesh ? 1 : 0);
            internalOptions.AutoRebuildNavMeshTimeoutMs = Options.General.AutoRebuildNavMeshTimeoutMs;
            Editor.Internal_SetOptions(ref internalOptions);

            EditorAssets.Cache.OnEditorOptionsChanged(Options);

            // Send event
            OptionsChanged?.Invoke(Options);
        }

        private void SetupStyle()
        {
            var themeOptions = Options.Theme;

            // If a non-default style was chosen, switch to that style
            string styleName = themeOptions.SelectedStyle;
            if (styleName != "Default" && themeOptions.Styles.TryGetValue(styleName, out var style) && style != null)
            {
                Style.Current = style;
            }
            else
            {
                Style.Current = CreateDefaultStyle();
            }
        }

        /// <summary>
        /// Creates the default style.
        /// </summary>
        /// <returns>The style object.</returns>
        public Style CreateDefaultStyle()
        {
            // Metro Style colors
            var options = Options;
            var style = new Style
            {
                Background = Color.FromBgra(0xFF1C1C1C),
                LightBackground = Color.FromBgra(0xFF2D2D30),
                Foreground = Color.FromBgra(0xFFFFFFFF),
                ForegroundGrey = Color.FromBgra(0xFFA9A9B3),
                ForegroundDisabled = Color.FromBgra(0xFF787883),
                BackgroundHighlighted = Color.FromBgra(0xFF54545C),
                BorderHighlighted = Color.FromBgra(0xFF6A6A75),
                BackgroundSelected = Color.FromBgra(0xFF007ACC),
                BorderSelected = Color.FromBgra(0xFF1C97EA),
                BackgroundNormal = Color.FromBgra(0xFF3F3F46),
                BorderNormal = Color.FromBgra(0xFF54545C),
                TextBoxBackground = Color.FromBgra(0xFF333337),
                TextBoxBackgroundSelected = Color.FromBgra(0xFF3F3F46),
                ProgressNormal = Color.FromBgra(0xFF0ad328),

                // Fonts
                FontTitle = options.Interface.TitleFont.GetFont(),
                FontLarge = options.Interface.LargeFont.GetFont(),
                FontMedium = options.Interface.MediumFont.GetFont(),
                FontSmall = options.Interface.SmallFont.GetFont(),

                // Icons
                ArrowDown = Editor.Icons.ArrowDown12,
                ArrowRight = Editor.Icons.ArrowRight12,
                Search = Editor.Icons.Search12,
                Settings = Editor.Icons.Settings12,
                Cross = Editor.Icons.Cross12,
                CheckBoxIntermediate = Editor.Icons.CheckBoxIntermediate12,
                CheckBoxTick = Editor.Icons.CheckBoxTick12,
                StatusBarSizeGrip = Editor.Icons.StatusBarSizeGrip12,
                Translate = Editor.Icons.Translate16,
                Rotate = Editor.Icons.Rotate16,
                Scale = Editor.Icons.Scale16,

                SharedTooltip = new Tooltip()
            };
            style.DragWindow = style.BackgroundSelected * 0.7f;

            return style;
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            Editor.Log("Options file path: " + _optionsFilePath);
            Load();
            SetupStyle();
        }
    }
}
