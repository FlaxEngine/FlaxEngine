// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.Content.Settings;
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
            if (!File.Exists(_optionsFilePath))
            {
                Editor.LogWarning("Missing editor settings");
                return;
            }

            try
            {
                // Load asset
                var asset = FlaxEngine.Content.LoadAsync<JsonAsset>(_optionsFilePath);
                if (asset == null)
                {
                    Editor.LogWarning("Invalid editor settings");
                    return;
                }
                if (asset.WaitForLoaded())
                {
                    Editor.LogError("Failed to load editor settings");
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

                    float prevInterfaceScale = Options.Interface.InterfaceScale;
                    Options = options;
                    OnOptionsChanged();

                    // Scale interface relative to the current value (eg. when using system-provided Dpi Scale)
                    Platform.CustomDpiScale *= Options.Interface.InterfaceScale / prevInterfaceScale;
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
            if (Editor.SaveJsonAsset(_optionsFilePath, Options))
            {
                MessageBox.Show(string.Format("Failed to save editor option to '{0}'. Ensure that directory exists and program has access to it.", _optionsFilePath), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            // Special case for editor analytics
            var editorAnalyticsTrackingFile = Path.Combine(Editor.LocalCachePath, "noTracking");
            if (Options.General.EnableEditorAnalytics)
            {
                if (File.Exists(editorAnalyticsTrackingFile))
                {
                    File.Delete(editorAnalyticsTrackingFile);
                }
            }
            else
            {
                if (!File.Exists(editorAnalyticsTrackingFile))
                {
                    File.WriteAllText(editorAnalyticsTrackingFile, "Don't track me, please.");
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
            internalOptions.UseAssetImportPathRelative = (byte)(Options.General.UseAssetImportPathRelative ? 1 : 0);
            internalOptions.EnableParticlesPreview = (byte)(Options.Visual.EnableParticlesPreview ? 1 : 0);
            internalOptions.AutoRebuildCSG = (byte)(Options.General.AutoRebuildCSG ? 1 : 0);
            internalOptions.AutoRebuildCSGTimeoutMs = Options.General.AutoRebuildCSGTimeoutMs;
            internalOptions.AutoRebuildNavMesh = (byte)(Options.General.AutoRebuildNavMesh ? 1 : 0);
            internalOptions.AutoRebuildNavMeshTimeoutMs = Options.General.AutoRebuildNavMeshTimeoutMs;
            Editor.Internal_SetOptions(ref internalOptions);

            EditorAssets.Cache.OnEditorOptionsChanged(Options);

            // Units formatting options
            bool useUnitsFormatting = Options.Interface.ValueFormatting != InterfaceOptions.ValueFormattingType.None;
            bool automaticUnitsFormatting = Options.Interface.ValueFormatting == InterfaceOptions.ValueFormattingType.AutoUnit;
            bool separateValueAndUnit = Options.Interface.SeparateValueAndUnit;
            if (useUnitsFormatting != Utilities.Units.UseUnitsFormatting ||
                automaticUnitsFormatting != Utilities.Units.AutomaticUnitsFormatting ||
                separateValueAndUnit != Utilities.Units.SeparateValueAndUnit)
            {
                Utilities.Units.UseUnitsFormatting = useUnitsFormatting;
                Utilities.Units.AutomaticUnitsFormatting = automaticUnitsFormatting;
                Utilities.Units.SeparateValueAndUnit = separateValueAndUnit;

                // Refresh UI in property panels
                Editor.Windows.PropertiesWin?.Presenter.BuildLayoutOnUpdate();
                foreach (var window in Editor.Windows.Windows)
                {
                    if (window is Windows.Assets.PrefabWindow prefabWindow)
                        prefabWindow.Presenter.BuildLayoutOnUpdate();
                }
            }

            // Send event
            OptionsChanged?.Invoke(Options);
        }

        private void SetupStyle()
        {
            var themeOptions = Options.Theme;

            // If a non-default style was chosen, switch to that style
            string styleName = themeOptions.SelectedStyle;
            if (styleName != ThemeOptions.DefaultName && styleName != ThemeOptions.LightDefault && themeOptions.Styles.TryGetValue(styleName, out var style) && style != null)
            {
                // Setup defaults for newly added components that might be missing
                if (style.Selection == Color.Transparent && style.SelectionBorder == Color.Transparent)
                {
                    // [Deprecated on 6.03.2024, expires on 6.03.2026]
                    style.Selection = Color.Orange * 0.4f;
                    style.SelectionBorder = Color.Orange;
                }

                Style.Current = style;
            }
            else
            {
                if (styleName == ThemeOptions.LightDefault)
                {
                    Style.Current = CreateLightStyle();
                }
                else
                {
                    Style.Current = CreateDefaultStyle();
                }
            }

            // Ensure custom fonts are valid, reset if not
            var defaultInterfaceOptions = new InterfaceOptions();
            if (Style.Current.FontTitle == null)
                Style.Current.FontTitle = defaultInterfaceOptions.TitleFont.GetFont();
            if (Style.Current.FontSmall == null)
                Style.Current.FontSmall = defaultInterfaceOptions.SmallFont.GetFont();
            if (Style.Current.FontMedium == null)
                Style.Current.FontMedium = defaultInterfaceOptions.MediumFont.GetFont();
            if (Style.Current.FontLarge == null)
                Style.Current.FontLarge = defaultInterfaceOptions.LargeFont.GetFont();

            // Set fallback fonts
            var fallbackFonts = Options.Interface.FallbackFonts;
            if (fallbackFonts == null || fallbackFonts.Length == 0 || fallbackFonts.All(x => x == null))
                fallbackFonts = GameSettings.Load<GraphicsSettings>().FallbackFonts;
            Font.FallbackFonts = fallbackFonts;
        }

        /// <summary>
        /// Creates the default style.
        /// </summary>
        /// <returns>The style object.</returns>
        public Style CreateDefaultStyle()
        {
            var options = Options;
            var style = new Style
            {
                Background = Color.FromBgra(0xFF1C1C1C),
                LightBackground = Color.FromBgra(0xFF2D2D30),
                Foreground = Color.FromBgra(0xFFFFFFFF),
                ForegroundGrey = Color.FromBgra(0xFFA9A9B3),
                ForegroundDisabled = Color.FromBgra(0xFF787883),
                ForegroundViewport = Color.FromBgra(0xFFFFFFFF),
                BackgroundHighlighted = Color.FromBgra(0xFF54545C),
                BorderHighlighted = Color.FromBgra(0xFF6A6A75),
                BackgroundSelected = Color.FromBgra(0xFF007ACC),
                BorderSelected = Color.FromBgra(0xFF1C97EA),
                BackgroundNormal = Color.FromBgra(0xFF3F3F46),
                BorderNormal = Color.FromBgra(0xFF54545C),
                TextBoxBackground = Color.FromBgra(0xFF333337),
                TextBoxBackgroundSelected = Color.FromBgra(0xFF3F3F46),
                CollectionBackgroundColor = Color.FromBgra(0x14CCCCCC),
                ProgressNormal = Color.FromBgra(0xFF0ad328),
                Selection = Color.Orange * 0.4f,
                SelectionBorder = Color.Orange,

                Statusbar = new Style.StatusbarStyle
                {
                    PlayMode = Color.FromBgra(0xFF2F9135),
                    Failed = Color.FromBgra(0xFF9C2424),
                    Loading = Color.FromBgra(0xFF2D2D30),
                },

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
                StatusBarSizeGrip = Editor.Icons.WindowDrag12,
                Translate = Editor.Icons.Translate32,
                Rotate = Editor.Icons.Rotate32,
                Scale = Editor.Icons.Scale32,
                Scalar = Editor.Icons.Scalar32,

                SharedTooltip = new Tooltip(),
            };
            style.SharedTooltip.HorizontalTextAlignment = Editor.Instance.Options.Options.Interface.TooltipTextAlignment;
            style.DragWindow = style.BackgroundSelected * 0.7f;
            return style;
        }

        /// <summary>
        /// Creates the light style (2nd default).
        /// </summary>
        /// <returns>The style object.</returns>
        public Style CreateLightStyle()
        {
            var options = Options;
            var style = new Style
            {
                Background = new Color(0.92f, 0.92f, 0.92f, 1f),
                LightBackground = new Color(0.84f, 0.84f, 0.88f, 1f),
                DragWindow = new Color(0.0f, 0.26f, 0.43f, 0.70f),
                Foreground = new Color(0.0f, 0.0f, 0.0f, 1f),
                ForegroundGrey = new Color(0.30f, 0.30f, 0.31f, 1f),
                ForegroundDisabled = new Color(0.45f, 0.45f, 0.49f, 1f),
                ForegroundViewport = new Color(1.0f, 1.0f, 1.0f, 1f),
                BackgroundHighlighted = new Color(0.59f, 0.59f, 0.64f, 1f),
                BorderHighlighted = new Color(0.50f, 0.50f, 0.55f, 1f),
                BackgroundSelected = new Color(0.00f, 0.46f, 0.78f, 0.78f),
                BorderSelected = new Color(0.11f, 0.57f, 0.88f, 0.65f),
                BackgroundNormal = new Color(0.67f, 0.67f, 0.75f, 1f),
                BorderNormal = new Color(0.59f, 0.59f, 0.64f, 1f),
                TextBoxBackground = new Color(0.75f, 0.75f, 0.81f, 1f),
                TextBoxBackgroundSelected = new Color(0.73f, 0.73f, 0.80f, 1f),
                CollectionBackgroundColor = new Color(0.85f, 0.85f, 0.88f, 1f),
                ProgressNormal = new Color(0.03f, 0.65f, 0.12f, 1f),
                Selection = Color.Orange * 0.4f,
                SelectionBorder = Color.Orange,

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
                StatusBarSizeGrip = Editor.Icons.WindowDrag12,
                Translate = Editor.Icons.Translate32,
                Rotate = Editor.Icons.Rotate32,
                Scale = Editor.Icons.Scale32,
                Scalar = Editor.Icons.Scalar32,

                SharedTooltip = new Tooltip(),
            };
            style.SharedTooltip.HorizontalTextAlignment = Editor.Instance.Options.Options.Interface.TooltipTextAlignment;
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
