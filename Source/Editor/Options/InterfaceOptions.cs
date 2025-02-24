// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.ComponentModel;
using FlaxEditor.GUI.Docking;
using FlaxEngine;
using FlaxEngine.GUI;

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
        /// Options focus Game Window behaviour when play mode is entered.
        /// </summary>
        public enum PlayModeFocus
        {
            /// <summary>
            /// Don't change focus.
            /// </summary>
            None,

            /// <summary>
            /// Focus the Game Window.
            /// </summary>
            GameWindow,

            /// <summary>
            /// Focus the Game Window. On play mode end restore focus to the previous window.
            /// </summary>
            GameWindowThenRestore,
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
        /// If checked, color pickers will always modify the color unless 'Cancel' if pressed, otherwise color won't change unless 'Ok' is pressed.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Interface"), EditorOrder(290)]
        public bool AutoAcceptColorPickerChange { get; set; } = true;

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
        /// Gets or sets tree line visibility.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Interface"), EditorOrder(320), Tooltip("Toggles tree line visibility in places like the Scene or Content Panel.")]
        public bool ShowTreeLines { get; set; } = true;

        /// <summary>
        /// Gets or sets tooltip text alignment.
        /// </summary>
        [DefaultValue(TextAlignment.Center)]
        [EditorDisplay("Interface"), EditorOrder(321)]
        public TextAlignment TooltipTextAlignment
        {
            get => _tooltipTextAlignment;
            set
            {
                _tooltipTextAlignment = value;
                var tooltip = Style.Current?.SharedTooltip;
                if (tooltip != null)
                    tooltip.HorizontalTextAlignment = value;
            }
        }

        private TextAlignment _tooltipTextAlignment = TextAlignment.Center;

        /// <summary>
        /// Whether to scroll to the script when a script is added to an actor.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Interface"), EditorOrder(322)]
        public bool ScrollToScriptOnAdd { get; set; } = true;

        /// <summary>
        /// Gets or sets the timestamps prefix mode for output log messages.
        /// </summary>
        [DefaultValue(TimestampsFormats.None)]
        [EditorDisplay("Debug Log"), EditorOrder(350), Tooltip("The timestamps prefix mode for debug log messages.")]
        public TimestampsFormats DebugLogTimestampsFormat { get; set; } = TimestampsFormats.None;

        /// <summary>
        /// Gets or sets the clear on play for debug log messages.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Debug Log", "Clear on Play"), EditorOrder(360), Tooltip("Clears all log entries on enter playmode.")]
        public bool DebugLogClearOnPlay { get; set; } = true;

        /// <summary>
        /// Gets or sets the collapse mode for debug log messages.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Debug Log"), EditorOrder(361), Tooltip("Collapses similar or repeating log entries.")]
        public bool DebugLogCollapse { get; set; } = true;

        /// <summary>
        /// Gets or sets the automatic pause on error for debug log messages.
        /// </summary>
        [DefaultValue(false)]
        [EditorDisplay("Debug Log", "Pause on Error"), EditorOrder(362), Tooltip("Performs auto pause on error.")]
        public bool DebugLogPauseOnError { get; set; } = false;

        /// <summary>
        /// Gets or sets the automatic pause on error for debug log messages.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Debug Log", "Show error messages"), EditorOrder(370), Tooltip("Shows/hides error messages.")]
        public bool DebugLogShowErrorMessages { get; set; } = true;

        /// <summary>
        /// Gets or sets the automatic pause on error for debug log messages.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Debug Log", "Show warning messages"), EditorOrder(371), Tooltip("Shows/hides warning messages.")]
        public bool DebugLogShowWarningMessages { get; set; } = true;

        /// <summary>
        /// Gets or sets the automatic pause on error for debug log messages.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Debug Log", "Show info messages"), EditorOrder(372), Tooltip("Shows/hides info messages.")]
        public bool DebugLogShowInfoMessages { get; set; } = true;

        /// <summary>
        /// Gets or sets the timestamps prefix mode for output log messages.
        /// </summary>
        [DefaultValue(TimestampsFormats.TimeSinceStartup)]
        [EditorDisplay("Output Log", "Timestamps Format"), EditorOrder(400), Tooltip("The timestamps prefix mode for output log messages.")]
        public TimestampsFormats OutputLogTimestampsFormat { get; set; } = TimestampsFormats.TimeSinceStartup;

        /// <summary>
        /// Gets or sets the log type prefix mode for output log messages.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Output Log", "Show Log Type"), EditorOrder(410), Tooltip("Determines whether show log type prefix in output log messages.")]
        public bool OutputLogShowLogType { get; set; } = true;

        /// <summary>
        /// Gets or sets the output log text font.
        /// </summary>
        [EditorDisplay("Output Log", "Text Font"), EditorOrder(420), Tooltip("The output log text font.")]
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
        [EditorDisplay("Output Log", "Text Color"), EditorOrder(430), Tooltip("The output log text color.")]
        public Color OutputLogTextColor { get; set; } = Color.White;

        /// <summary>
        /// Gets or sets the output log text shadow color.
        /// </summary>
        [DefaultValue(typeof(Color), "0,0,0,0.5")]
        [EditorDisplay("Output Log", "Text Shadow Color"), EditorOrder(440), Tooltip("The output log text shadow color.")]
        public Color OutputLogTextShadowColor { get; set; } = new Color(0, 0, 0, 0.5f);

        /// <summary>
        /// Gets or sets the output log text shadow offset. Set to 0 to disable this feature.
        /// </summary>
        [DefaultValue(typeof(Float2), "1,1")]
        [EditorDisplay("Output Log", "Text Shadow Offset"), EditorOrder(445), Tooltip("The output log text shadow offset. Set to 0 to disable this feature.")]
        public Float2 OutputLogTextShadowOffset { get; set; } = new Float2(1);

        // [Deprecated in v1.10]
        [Serialize, Obsolete, NoUndo]
        private bool FocusGameWinOnPlay
        {
            get => throw new Exception();
            set
            {
                // Upgrade value
                FocusOnPlayMode = value ? PlayModeFocus.GameWindow : PlayModeFocus.None;
            }
        }

        /// <summary>
        /// Gets or sets the output log text color for warnings
        /// </summary>
        [DefaultValue(typeof(Color), "1,1,0,1")]
        [EditorDisplay("Output Log", "Warning Color"), EditorOrder(446), Tooltip("The output log text color for warnings.")]
        public Color OutputLogWarningTextColor { get; set; } = Color.Yellow;


        /// <summary>
        /// Gets or sets the output log text color for errors
        /// </summary>
        [DefaultValue(typeof(Color), "1,0,0,1")]
        [EditorDisplay("Output Log", "Error Color"), EditorOrder(445), Tooltip("The output log text color for errors.")]
        public Color OutputLogErrorTextColor { get; set; } = Color.Red;


        /// <summary>
        /// Gets or sets a value indicating whether auto-focus output log window on code compilation error.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Output Log", "Focus Output Log On Compilation Error"), EditorOrder(450), Tooltip("Determines whether auto-focus output log window on code compilation error.")]
        public bool FocusOutputLogOnCompilationError { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether auto-focus output log window on game build error.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Output Log", "Focus Output Log On Game Build Error"), EditorOrder(460), Tooltip("Determines whether auto-focus output log window on game build error.")]
        public bool FocusOutputLogOnGameBuildError { get; set; } = true;

        /// <summary>
        /// Gets or sets the value for automatic scroll to bottom in output log.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Output Log", "Scroll to bottom"), EditorOrder(470), Tooltip("Scroll the output log view to bottom automatically after new lines are added.")]
        public bool OutputLogScrollToBottom { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating what panel should be focused when play mode start.
        /// </summary>
        [DefaultValue(PlayModeFocus.GameWindow)]
        [EditorDisplay("Play In-Editor", "Focus On Play"), EditorOrder(500), Tooltip("Set what panel to focus on play mode start.")]
        public PlayModeFocus FocusOnPlayMode { get; set; } = PlayModeFocus.GameWindow;

        /// <summary>
        /// Gets or sets a value indicating what action should be taken upon pressing the play button.
        /// </summary>
        [DefaultValue(PlayAction.PlayScenes)]
        [EditorDisplay("Play In-Editor", "Play Button Action"), EditorOrder(510)]
        public PlayAction PlayButtonAction { get; set; } = PlayAction.PlayScenes;

        /// <summary>
        /// Gets or sets a value indicating how the game window should be displayed when the game is launched.
        /// </summary>
        [DefaultValue(GameWindowMode.Docked)]
        [EditorDisplay("Play In-Editor", "Game Window Mode"), EditorOrder(520), Tooltip("Determines how the game window is displayed when the game is launched.")]
        public GameWindowMode DefaultGameWindowMode { get; set; } = GameWindowMode.Docked;

        /// <summary>
        /// Gets or sets a value indicating the number of game clients to launch when building and/or running cooked game.
        /// </summary>
        [DefaultValue(1), Range(1, 4)]
        [EditorDisplay("Cook & Run"), EditorOrder(600)]
        public int NumberOfGameClientsToLaunch = 1;

        /// <summary>
        /// Gets or sets the curvature of the line connecting to connected visject nodes.
        /// </summary>
        [DefaultValue(1.0f), Range(0.0f, 2.0f)]
        [EditorDisplay("Visject"), EditorOrder(550)]
        public float ConnectionCurvature { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets a value that indicates wether the context menu description panel is shown or not.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Visject"), EditorOrder(550), Tooltip("Shows/hides the description panel in visual scripting context menu.")]
        public bool NodeDescriptionPanel { get; set; } = true;

        /// <summary>
        /// Gets or sets the surface grid snapping option.
        /// </summary>
        [DefaultValue(false)]
        [EditorDisplay("Visject", "Grid Snapping"), EditorOrder(551), Tooltip("Toggles grid snapping when moving nodes.")]
        public bool SurfaceGridSnapping { get; set; } = false;

        /// <summary>
        /// Gets or sets the surface grid snapping option.
        /// </summary>
        [DefaultValue(20.0f)]
        [EditorDisplay("Visject", "Grid Snapping Size"), EditorOrder(551), Tooltip("Defines the size of the grid for nodes snapping."), VisibleIf(nameof(SurfaceGridSnapping))]
        public float SurfaceGridSnappingSize { get; set; } = 20.0f;

        /// <summary>
        /// Gets or sets a value that indicates if a warning should be displayed when deleting a Visject parameter that is used in a graph.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Visject", "Warn when deleting used parameter"), EditorOrder(552)]
        public bool WarnOnDeletingUsedVisjectParameter { get; set; } = true;

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
        [EditorDisplay("Fonts"), EditorOrder(750)]
        public FontAsset[] FallbackFonts = new FontAsset[1] { FlaxEngine.Content.LoadAsyncInternal<FontAsset>(EditorAssets.FallbackFont) };

        /// <summary>
        /// Gets or sets the title font for editor UI.
        /// </summary>
        [EditorDisplay("Fonts"), EditorOrder(700), Tooltip("The title font for editor UI.")]
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
        [EditorDisplay("Fonts"), EditorOrder(710), Tooltip("The large font for editor UI.")]
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
        [EditorDisplay("Fonts"), EditorOrder(720), Tooltip("The medium font for editor UI.")]
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
        [EditorDisplay("Fonts"), EditorOrder(730), Tooltip("The small font for editor UI.")]
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
