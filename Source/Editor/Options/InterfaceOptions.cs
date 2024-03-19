// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using FlaxEditor.GUI.Docking;
using FlaxEditor.Utilities;
using FlaxEngine;

namespace FlaxEditor.Options
{
    /// <summary>
    /// Editor interface options data container.
    /// </summary>
    [CustomEditor(typeof(Editor<InterfaceOptions>))]
    public class InterfaceOptions
    {
        /// <summary>
        /// The log timestamp modes.
        /// </summary>
        public enum TimestampsFormats
        {
            /// <summary>
            /// No prefix.
            /// </summary>
            None,

            /// <summary>
            /// The UTC time format.
            /// </summary>
            Utc,

            /// <summary>
            /// The local time format.
            /// </summary>
            LocalTime,

            /// <summary>
            /// The time since startup (in seconds).
            /// </summary>
            TimeSinceStartup,
        }

        /// <summary>
        /// A proxy DockState for window open method.
        /// </summary>
        public enum DockStateProxy
        {
            /// <summary>
            /// The floating window.
            /// </summary>
            Float = DockState.Float,

            /// <summary>
            /// The dock fill as a tab.
            /// </summary>
            DockFill = DockState.DockFill,

            /// <summary>
            /// The dock left.
            /// </summary>
            DockLeft = DockState.DockLeft,

            /// <summary>
            /// The dock right.
            /// </summary>
            DockRight = DockState.DockRight,

            /// <summary>
            /// The dock top.
            /// </summary>
            DockTop = DockState.DockTop,

            /// <summary>
            /// The dock bottom.
            /// </summary>
            DockBottom = DockState.DockBottom
        }

        /// <summary>
        /// Options for the action taken by the play button.
        /// </summary>
        public enum PlayAction
        {
            /// <summary>
            /// Launches the game from the First Scene defined in the project settings.
            /// </summary>
            PlayGame,

            /// <summary>
            /// Launches the game using the scenes currently loaded in the editor.
            /// </summary>
            PlayScenes,
        }

        /// <summary>
        /// Available window modes for the game window.
        /// </summary>
        public enum GameWindowMode
        {
            /// <summary>
            /// Shows the game window docked, inside the editor.
            /// </summary>
            Docked,

            /// <summary>
            /// Shows the game window as a popup.
            /// </summary>
            PopupWindow,

            /// <summary>
            /// Shows the game window maximized. (Same as pressing F11)
            /// </summary>
            MaximizedWindow,

            /// <summary>
            /// Shows the game window borderless.
            /// </summary>
            BorderlessWindow,
        }

        /// <summary>
        /// Options for formatting numerical values.
        /// </summary>
        public enum ValueFormattingType
        {
            /// <summary>
            /// No formatting.
            /// </summary>
            None,

            /// <summary>
            /// Format using the base SI unit.
            /// </summary>
            BaseUnit,

            /// <summary>
            /// Format using a unit that matches the value best.
            /// </summary>
            AutoUnit,
        }

        /// <summary>
        /// Gets or sets the Editor User Interface scale. Applied to all UI elements, windows and text. Can be used to scale the interface up on a bigger display. Editor restart required.
        /// </summary>
        [DefaultValue(1.0f), Limit(0.1f, 10.0f)]
        [EditorDisplay("Interface"), EditorOrder(10), Tooltip("Editor User Interface scale. Applied to all UI elements, windows and text. Can be used to scale the interface up on a bigger display. Editor restart required.")]
        public float InterfaceScale { get; set; } = 1.0f;

#if PLATFORM_WINDOWS
        /// <summary>
        /// Gets or sets a value indicating whether use native window title bar. Editor restart required.
        /// </summary>
        [DefaultValue(false)]
        [EditorDisplay("Interface"), EditorOrder(70), Tooltip("Determines whether use native window title bar. Editor restart required.")]
        public bool UseNativeWindowSystem { get; set; } = false;
#endif

        /// <summary>
        /// Gets or sets a value indicating whether show selected camera preview in the editor window.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Interface"), EditorOrder(80), Tooltip("Determines whether show selected camera preview in the edit window.")]
        public bool ShowSelectedCameraPreview { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether center mouse position on window focus in play mode. Helps when working with games that lock mouse cursor.
        /// </summary>
        [DefaultValue(false)]
        [EditorDisplay("Interface", "Center Mouse On Game Window Focus"), EditorOrder(100), Tooltip("Determines whether center mouse position on window focus in play mode. Helps when working with games that lock mouse cursor.")]
        public bool CenterMouseOnGameWinFocus { get; set; } = false;

        /// <summary>
        /// Gets or sets the method window opening.
        /// </summary>
        [DefaultValue(DockStateProxy.Float)]
        [EditorDisplay("Interface", "New Window Location"), EditorOrder(150), Tooltip("Define the opening method for new windows, open in a new tab by default.")]
        public DockStateProxy NewWindowLocation { get; set; } = DockStateProxy.Float;

        /// <summary>
        /// Gets or sets the timestamps prefix mode for debug log messages.
        /// </summary>
        [DefaultValue(TimestampsFormats.None)]
        [EditorDisplay("Interface"), EditorOrder(210), Tooltip("The timestamps prefix mode for debug log messages.")]
        public TimestampsFormats DebugLogTimestampsFormat { get; set; } = TimestampsFormats.None;

        /// <summary>
        /// Gets or sets the editor icons scale. Editor restart required.
        /// </summary>
        [DefaultValue(1.0f), Limit(0.1f, 4.0f, 0.01f)]
        [EditorDisplay("Interface"), EditorOrder(250), Tooltip("Editor icons scale. Editor restart required.")]
        public float IconsScale { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the editor content window orientation.
        /// </summary>
        [DefaultValue(FlaxEngine.GUI.Orientation.Horizontal)]
        [EditorDisplay("Interface"), EditorOrder(280), Tooltip("Editor content window orientation.")]
        public FlaxEngine.GUI.Orientation ContentWindowOrientation { get; set; } = FlaxEngine.GUI.Orientation.Horizontal;

        /// <summary>
        /// Gets or sets the formatting option for numeric values in the editor.
        /// </summary>
        [DefaultValue(ValueFormattingType.None)]
        [EditorDisplay("Interface"), EditorOrder(300)]
        public ValueFormattingType ValueFormatting { get; set; }

        /// <summary>
        /// Gets or sets the option to put a space between numbers and units for unit formatting.
        /// </summary>
        [DefaultValue(false)]
        [EditorDisplay("Interface"), EditorOrder(310)]
        public bool SeparateValueAndUnit { get; set; }

        /// <summary>
        /// Gets or sets the timestamps prefix mode for output log messages.
        /// </summary>
        [DefaultValue(TimestampsFormats.TimeSinceStartup)]
        [EditorDisplay("Output Log", "Timestamps Format"), EditorOrder(300), Tooltip("The timestamps prefix mode for output log messages.")]
        public TimestampsFormats OutputLogTimestampsFormat { get; set; } = TimestampsFormats.TimeSinceStartup;

        /// <summary>
        /// Gets or sets the timestamps prefix mode for output log messages.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Output Log", "Show Log Type"), EditorOrder(310), Tooltip("Determines whether show log type prefix in output log messages.")]
        public bool OutputLogShowLogType { get; set; } = true;

        /// <summary>
        /// Gets or sets the output log text font.
        /// </summary>
        [EditorDisplay("Output Log", "Text Font"), EditorOrder(320), Tooltip("The output log text font.")]
        public FontReference OutputLogTextFont
        {
            get => _outputLogFont;
            set
            {
                if (value == null)
                    _outputLogFont = new FontReference(ConsoleFont, 10);
                else if (!value.Font)
                    _outputLogFont.Font = ConsoleFont;
                else
                    _outputLogFont = value;
            }
        }

        /// <summary>
        /// Gets or sets the output log text color.
        /// </summary>
        [DefaultValue(typeof(Color), "1,1,1,1")]
        [EditorDisplay("Output Log", "Text Color"), EditorOrder(330), Tooltip("The output log text color.")]
        public Color OutputLogTextColor { get; set; } = Color.White;

        /// <summary>
        /// Gets or sets the output log text shadow color.
        /// </summary>
        [DefaultValue(typeof(Color), "0,0,0,0.5")]
        [EditorDisplay("Output Log", "Text Shadow Color"), EditorOrder(340), Tooltip("The output log text shadow color.")]
        public Color OutputLogTextShadowColor { get; set; } = new Color(0, 0, 0, 0.5f);

        /// <summary>
        /// Gets or sets the output log text shadow offset. Set to 0 to disable this feature.
        /// </summary>
        [DefaultValue(typeof(Float2), "1,1")]
        [EditorDisplay("Output Log", "Text Shadow Offset"), EditorOrder(340), Tooltip("The output log text shadow offset. Set to 0 to disable this feature.")]
        public Float2 OutputLogTextShadowOffset { get; set; } = new Float2(1);

        /// <summary>
        /// Gets or sets a value indicating whether auto-focus output log window on code compilation error.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Output Log", "Focus Output Log On Compilation Error"), EditorOrder(350), Tooltip("Determines whether auto-focus output log window on code compilation error.")]
        public bool FocusOutputLogOnCompilationError { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether auto-focus output log window on game build error.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Output Log", "Focus Output Log On Game Build Error"), EditorOrder(360), Tooltip("Determines whether auto-focus output log window on game build error.")]
        public bool FocusOutputLogOnGameBuildError { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether auto-focus game window on play mode start.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Play In-Editor", "Focus Game Window On Play"), EditorOrder(400), Tooltip("Determines whether auto-focus game window on play mode start.")]
        public bool FocusGameWinOnPlay { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating what action should be taken upon pressing the play button.
        /// </summary>
        [DefaultValue(PlayAction.PlayScenes)]
        [EditorDisplay("Play In-Editor", "Play Button Action"), EditorOrder(410)]
        public PlayAction PlayButtonAction { get; set; } = PlayAction.PlayScenes;

        /// <summary>
        /// Gets or sets a value indicating how the game window should be displayed when the game is launched.
        /// </summary>
        [DefaultValue(GameWindowMode.Docked)]
        [EditorDisplay("Play In-Editor", "Game Window Mode"), EditorOrder(420), Tooltip("Determines how the game window is displayed when the game is launched.")]
        public GameWindowMode DefaultGameWindowMode { get; set; } = GameWindowMode.Docked;

        /// <summary>
        /// Gets or sets a value indicating the number of game clients to launch when building and/or running cooked game.
        /// </summary>
        [DefaultValue(1), Range(1, 4)]
        [EditorDisplay("Cook & Run"), EditorOrder(500)]
        public int NumberOfGameClientsToLaunch = 1;

        private static FontAsset DefaultFont => FlaxEngine.Content.LoadAsyncInternal<FontAsset>(EditorAssets.PrimaryFont);
        private static FontAsset ConsoleFont => FlaxEngine.Content.LoadAsyncInternal<FontAsset>(EditorAssets.InconsolataRegularFont);

        private FontReference _titleFont = new FontReference(DefaultFont, 18);
        private FontReference _largeFont = new FontReference(DefaultFont, 14);
        private FontReference _mediumFont = new FontReference(DefaultFont, 9);
        private FontReference _smallFont = new FontReference(DefaultFont, 9);
        private FontReference _outputLogFont = new FontReference(ConsoleFont, 10);

        /// <summary>
        /// The list of fallback fonts to use when main text font is missing certain characters. Empty to use fonts from GraphicsSettings.
        /// </summary>
        [EditorDisplay("Fonts"), EditorOrder(650)]
        public FontAsset[] FallbackFonts = new FontAsset[1] { FlaxEngine.Content.LoadAsyncInternal<FontAsset>(EditorAssets.FallbackFont) };

        /// <summary>
        /// Gets or sets the title font for editor UI.
        /// </summary>
        [EditorDisplay("Fonts"), EditorOrder(600), Tooltip("The title font for editor UI.")]
        public FontReference TitleFont
        {
            get => _titleFont;
            set
            {
                if (value == null)
                    _titleFont = new FontReference(DefaultFont, 18);
                else if (!value.Font)
                    _titleFont.Font = DefaultFont;
                else
                    _titleFont = value;
            }
        }

        /// <summary>
        /// Gets or sets the large font for editor UI.
        /// </summary>
        [EditorDisplay("Fonts"), EditorOrder(610), Tooltip("The large font for editor UI.")]
        public FontReference LargeFont
        {
            get => _largeFont;
            set
            {
                if (value == null)
                    _largeFont = new FontReference(DefaultFont, 14);
                else if (!value.Font)
                    _largeFont.Font = DefaultFont;
                else
                    _largeFont = value;
            }
        }

        /// <summary>
        /// Gets or sets the medium font for editor UI.
        /// </summary>
        [EditorDisplay("Fonts"), EditorOrder(620), Tooltip("The medium font for editor UI.")]
        public FontReference MediumFont
        {
            get => _mediumFont;
            set
            {
                if (value == null)
                    _mediumFont = new FontReference(DefaultFont, 9);
                else if (!value.Font)
                    _mediumFont.Font = DefaultFont;
                else
                    _mediumFont = value;
            }
        }

        /// <summary>
        /// Gets or sets the small font for editor UI.
        /// </summary>
        [EditorDisplay("Fonts"), EditorOrder(630), Tooltip("The small font for editor UI.")]
        public FontReference SmallFont
        {
            get => _smallFont;
            set
            {
                if (value == null)
                    _smallFont = new FontReference(DefaultFont, 9);
                else if (!value.Font)
                    _smallFont.Font = DefaultFont;
                else
                    _smallFont = value;
            }
        }
    }
}
