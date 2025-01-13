// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

// ReSharper disable InconsistentNaming
// ReSharper disable MemberCanBePrivate.Local
#pragma warning disable 649

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Editor tool window for building games using <see cref="GameCooker"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public sealed class GameCookerWindow : EditorWindow
    {
        /// <summary>
        /// Proxy object for the Build tab.
        /// </summary>
        [HideInEditor]
        [CustomEditor(typeof(BuildTabProxy.Editor))]
        private class BuildTabProxy
        {
            public readonly GameCookerWindow GameCookerWin;
            public readonly PlatformSelector Selector;

            internal readonly Dictionary<PlatformType, Platform> PerPlatformOptions = new Dictionary<PlatformType, Platform>
            {
                { PlatformType.Windows, new Windows() },
                { PlatformType.XboxOne, new XboxOne() },
                { PlatformType.UWP, new UWP() },
                { PlatformType.Linux, new Linux() },
                { PlatformType.PS4, new PS4() },
                { PlatformType.XboxScarlett, new XboxScarlett() },
                { PlatformType.Android, new Android() },
                { PlatformType.Switch, new Switch() },
                { PlatformType.PS5, new PS5() },
                { PlatformType.Mac, new Mac() },
                { PlatformType.iOS, new iOS() },
            };

            public BuildTabProxy(GameCookerWindow win, PlatformSelector platformSelector)
            {
                GameCookerWin = win;
                Selector = platformSelector;

                PerPlatformOptions[PlatformType.Windows].Init("Output/Windows", "Windows");
                PerPlatformOptions[PlatformType.XboxOne].Init("Output/XboxOne", "XboxOne");
                PerPlatformOptions[PlatformType.UWP].Init("Output/UWP", "UWP");
                PerPlatformOptions[PlatformType.Linux].Init("Output/Linux", "Linux");
                PerPlatformOptions[PlatformType.PS4].Init("Output/PS4", "PS4");
                PerPlatformOptions[PlatformType.XboxScarlett].Init("Output/XboxScarlett", "XboxScarlett");
                PerPlatformOptions[PlatformType.Android].Init("Output/Android", "Android");
                PerPlatformOptions[PlatformType.Switch].Init("Output/Switch", "Switch");
                PerPlatformOptions[PlatformType.PS5].Init("Output/PS5", "PS5");
                PerPlatformOptions[PlatformType.Mac].Init("Output/Mac", "Mac");
                PerPlatformOptions[PlatformType.iOS].Init("Output/iOS", "iOS");
            }

            [HideInEditor]
            internal abstract class Platform
            {
                [HideInEditor]
                public bool IsSupported;

                [HideInEditor]
                public bool IsAvailable;

                [EditorOrder(10), Tooltip("Output folder path")]
                public string Output;

                [EditorOrder(11), Tooltip("Show output folder in Explorer after build")]
                public bool ShowOutput = true;

                [EditorOrder(20), Tooltip("Configuration build mode")]
                public BuildConfiguration ConfigurationMode = BuildConfiguration.Development;

                [EditorOrder(90), Tooltip("The list of custom defines passed to the build tool when compiling project scripts. Can be used in build scripts for configuration (Configuration.CustomDefines).")]
                public string[] CustomDefines;

                protected abstract BuildPlatform BuildPlatform { get; }

                protected virtual BuildOptions Options
                {
                    get
                    {
                        BuildOptions options = BuildOptions.None;
                        if (ShowOutput)
                            options |= BuildOptions.ShowOutput;
                        return options;
                    }
                }

                public virtual void Init(string output, string platformDataSubDir)
                {
                    Output = output;

                    // Check if can build on that platform
#if PLATFORM_WINDOWS
                    switch (BuildPlatform)
                    {
                    case BuildPlatform.MacOSx64:
                    case BuildPlatform.MacOSARM64:
                    case BuildPlatform.iOSARM64:
                        IsSupported = false;
                        break;
                    default:
                        IsSupported = true;
                        break;
                    }
#elif PLATFORM_LINUX
                    switch (BuildPlatform)
                    {
                    case BuildPlatform.LinuxX64:
                    case BuildPlatform.AndroidARM64:
                        IsSupported = true;
                        break;
                    default:
                        IsSupported = false;
                        break;
                    }
#elif PLATFORM_MAC
                    switch (BuildPlatform)
                    {
                    case BuildPlatform.MacOSx64:
                    case BuildPlatform.MacOSARM64:
                    case BuildPlatform.iOSARM64:
                    case BuildPlatform.AndroidARM64:
                        IsSupported = true;
                        break;
                    default:
                        IsSupported = false;
                        break;
                    }
#else
#error "Unknown platform."
#endif

                    // TODO: restore build settings from the Editor cache!

                    // Check if can find installed tools for this platform
                    IsAvailable = Directory.Exists(Path.Combine(Globals.StartupFolder, "Source", "Platforms", platformDataSubDir, "Binaries"));
                }

                public virtual void OnNotAvailableLayout(LayoutElementsContainer layout)
                {
                    string text = "Missing platform data tools for the target platform.";
                    if (FlaxEditor.Editor.IsOfficialBuild())
                    {
                        switch (BuildPlatform)
                        {
#if PLATFORM_WINDOWS
                        case BuildPlatform.Windows32:
                        case BuildPlatform.Windows64:
                        case BuildPlatform.UWPx86:
                        case BuildPlatform.UWPx64:
                        case BuildPlatform.LinuxX64:
                        case BuildPlatform.AndroidARM64:
                            text += "\nUse Flax Launcher and download the required package.";
                            break;
#endif
                        default:
                            text += "\nEngine source is required to target this platform.";
                            break;
                        }
                    }
                    else
                    {
                        text += "\nTo target this platform separate engine source package is required.";
                        switch (BuildPlatform)
                        {
                        case BuildPlatform.XboxOne:
                        case BuildPlatform.XboxScarlett:
                        case BuildPlatform.PS4:
                        case BuildPlatform.PS5:
                        case BuildPlatform.Switch:
                            text += "\nTo get access please contact via https://flaxengine.com/contact";
                            break;
                        }
                    }
                    var label = layout.Label(text, TextAlignment.Center);
                    label.Label.AutoHeight = true;
                }

                public virtual void Build()
                {
                    var output = StringUtils.ConvertRelativePathToAbsolute(Globals.ProjectFolder, StringUtils.NormalizePath(Output));
                    GameCooker.Build(BuildPlatform, ConfigurationMode, output, Options, CustomDefines);
                }
            }

            class Windows : Platform
            {
                protected override BuildPlatform BuildPlatform => BuildPlatform.Windows64;
            }

            class UWP : Platform
            {
                protected override BuildPlatform BuildPlatform => BuildPlatform.UWPx64;
            }

            class XboxOne : Platform
            {
                protected override BuildPlatform BuildPlatform => BuildPlatform.XboxOne;
            }

            class Linux : Platform
            {
                protected override BuildPlatform BuildPlatform => BuildPlatform.LinuxX64;
            }

            class PS4 : Platform
            {
                protected override BuildPlatform BuildPlatform => BuildPlatform.PS4;
            }

            class XboxScarlett : Platform
            {
                protected override BuildPlatform BuildPlatform => BuildPlatform.XboxScarlett;
            }

            class Android : Platform
            {
                protected override BuildPlatform BuildPlatform => BuildPlatform.AndroidARM64;
            }

            class Switch : Platform
            {
                protected override BuildPlatform BuildPlatform => BuildPlatform.Switch;
            }

            class PS5 : Platform
            {
                protected override BuildPlatform BuildPlatform => BuildPlatform.PS5;
            }

            class Mac : Platform
            {
                public enum Archs
                {
                    [EditorDisplay(null, "arm64")]
                    ARM64,

                    [EditorDisplay(null, "x64")]
                    x64,
                }

                public Archs CPU = Archs.ARM64;

                protected override BuildPlatform BuildPlatform => CPU == Archs.ARM64 ? BuildPlatform.MacOSARM64 : BuildPlatform.MacOSx64;
            }

            class iOS : Platform
            {
                protected override BuildPlatform BuildPlatform => BuildPlatform.iOSARM64;
            }

            class Editor : CustomEditor
            {
                private PlatformType _platform;
                private Button _buildButton;

                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (BuildTabProxy)Values[0];
                    _platform = proxy.Selector.Selected;
                    var platformObj = proxy.PerPlatformOptions[_platform];

                    if (!platformObj.IsSupported)
                    {
                        layout.Label("This platform is not supported on this system.", TextAlignment.Center);
                    }
                    else if (platformObj.IsAvailable)
                    {
                        string name;
                        switch (_platform)
                        {
                        case PlatformType.Windows:
                            name = "Windows";
                            break;
                        case PlatformType.XboxOne:
                            name = "Xbox One";
                            break;
                        case PlatformType.UWP:
                            name = "Windows Store";
                            layout.Label("UWP (Windows Store) platform has been deprecated and is no longer supported", TextAlignment.Center).Label.TextColor = Color.Red;
                            break;
                        case PlatformType.Linux:
                            name = "Linux";
                            break;
                        case PlatformType.PS4:
                            name = "PlayStation 4";
                            break;
                        case PlatformType.XboxScarlett:
                            name = "Xbox Scarlett";
                            break;
                        case PlatformType.Android:
                            name = "Android";
                            break;
                        case PlatformType.Switch:
                            name = "Switch";
                            break;
                        case PlatformType.PS5:
                            name = "PlayStation 5";
                            break;
                        case PlatformType.Mac:
                            name = "Mac";
                            break;
                        case PlatformType.iOS:
                            name = "iOS";
                            break;
                        default:
                            name = Utilities.Utils.GetPropertyNameUI(_platform.ToString());
                            break;
                        }
                        var group = layout.Group(name);

                        group.Object(new ReadOnlyValueContainer(platformObj));

                        layout.Space(2);
                        var openOutputButton = layout.Button("Open output folder").Button;
                        openOutputButton.TooltipText = "Opens the defined out folder if the path exists.";
                        openOutputButton.Clicked += () =>
                        {
                            string output = StringUtils.ConvertRelativePathToAbsolute(Globals.ProjectFolder, StringUtils.NormalizePath(proxy.PerPlatformOptions[_platform].Output));
                            if (Directory.Exists(output))
                                FlaxEngine.FileSystem.ShowFileExplorer(output);
                            else
                                FlaxEditor.Editor.LogWarning($"Can not open path: {output} because it does not exist.");
                        };
                        layout.Space(2);

                        _buildButton = layout.Button("Build").Button;
                        _buildButton.Clicked += OnBuildClicked;
                    }
                    else
                    {
                        platformObj.OnNotAvailableLayout(layout);
                    }
                }

                private void OnBuildClicked()
                {
                    var proxy = (BuildTabProxy)Values[0];
                    var platformObj = proxy.PerPlatformOptions[_platform];
                    platformObj.Build();
                }

                public override void Refresh()
                {
                    base.Refresh();

                    if (_buildButton != null)
                    {
                        _buildButton.Enabled = !GameCooker.IsRunning;
                    }

                    if (Values.Count > 0 && Values[0] is BuildTabProxy proxy && proxy.Selector.Selected != _platform)
                    {
                        RebuildLayout();
                    }
                }
            }
        }

        private class PresetsTargetsColumnBase : ContainerControl
        {
            protected GameCookerWindow _cooker;

            protected PresetsTargetsColumnBase(ContainerControl parent, GameCookerWindow cooker, bool isPresets, Action addClicked)
            {
                AnchorPreset = AnchorPresets.VerticalStretchLeft;
                Parent = parent;
                Offsets = new Margin(isPresets ? 0 : 140, 140, 0, 0);

                _cooker = cooker;
                var title = new Label
                {
                    Bounds = new Rectangle(0, 0, Width, 19),
                    Text = isPresets ? "Presets" : "Targets",
                    Parent = this,
                };
                var addButton = new Button
                {
                    Text = isPresets ? "New preset" : "Add target",
                    Bounds = new Rectangle(6, 22, Width - 12, title.Bottom),
                    Parent = this,
                };
                addButton.Clicked += addClicked;
            }

            protected void RemoveButtons()
            {
                for (int i = ChildrenCount - 1; i >= 0; i--)
                {
                    if (Children[i].Tag != null)
                    {
                        Children[i].Dispose();
                    }
                }
            }

            protected void AddButton(string name, int index, int selectedIndex, Action<Button> select, Action<Button> remove)
            {
                var height = 26;
                var y = 52 + index * height;
                var selectButton = new Button
                {
                    Text = name,
                    Tag = index,
                    Parent = this,
                    Bounds = new Rectangle(6, y + 2, Width - 12 - 20, 22),
                };
                if (selectedIndex == index)
                    selectButton.SetColors(Color.FromBgra(0xFFAB8400));
                selectButton.ButtonClicked += select;
                var removeButton = new Button
                {
                    Text = "x",
                    Tag = index,
                    Parent = this,
                    Bounds = new Rectangle(selectButton.Right + 4, y + 4, 18, 18),
                };
                removeButton.ButtonClicked += remove;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var color = Style.Current.Background * 0.8f;
                Render2D.DrawLine(new Float2(Width, 0), new Float2(Width, Height), color);
                Render2D.DrawLine(new Float2(0, 48), new Float2(Width, 48), color);
            }
        }

        private sealed class PresetsColumn : PresetsTargetsColumnBase
        {
            public PresetsColumn(ContainerControl parent, GameCookerWindow cooker)
            : base(parent, cooker, true, cooker.AddPreset)
            {
                _cooker = cooker;
            }

            public void RefreshColumn(BuildPreset[] presets, int selectedIndex)
            {
                RemoveButtons();

                if (presets != null)
                {
                    for (int i = 0; i < presets.Length; i++)
                    {
                        if (presets[i] == null)
                            return;
                        AddButton(presets[i].Name, i, selectedIndex,
                                  b => _cooker.SelectPreset((int)b.Tag),
                                  b => _cooker.RemovePreset((int)b.Tag));
                    }
                }
            }
        }

        private sealed class TargetsColumn : PresetsTargetsColumnBase
        {
            public TargetsColumn(ContainerControl parent, GameCookerWindow cooker)
            : base(parent, cooker, false, cooker.AddTarget)
            {
                _cooker = cooker;

                var height = 26;
                var helpButton = new Button
                {
                    Text = "Help",
                    Parent = this,
                    AnchorPreset = AnchorPresets.BottomLeft,
                    Bounds = new Rectangle(6, Height - height, Width - 12, 22),
                };
                helpButton.Clicked += () => Platform.OpenUrl(Constants.DocsUrl + "manual/editor/game-cooker/");
                var buildAllButton = new Button
                {
                    Text = "Build All",
                    Parent = this,
                    AnchorPreset = AnchorPresets.BottomLeft,
                    Bounds = new Rectangle(6, helpButton.Top - height, Width - 12, 22),
                };
                buildAllButton.Clicked += _cooker.BuildAllTargets;
                var buildButton = new Button
                {
                    Text = "Build",
                    Parent = this,
                    AnchorPreset = AnchorPresets.BottomLeft,
                    Bounds = new Rectangle(6, buildAllButton.Top - height, Width - 12, 22),
                };
                buildButton.Clicked += _cooker.BuildTarget;
                var discardButton = new Button
                {
                    Text = "Discard",
                    Parent = this,
                    AnchorPreset = AnchorPresets.BottomLeft,
                    Bounds = new Rectangle(6, buildButton.Top - height, Width - 12, 22),
                };
                discardButton.Clicked += _cooker.GatherData;
                var saveButton = new Button
                {
                    Text = "Save",
                    Parent = this,
                    AnchorPreset = AnchorPresets.BottomLeft,
                    Bounds = new Rectangle(6, discardButton.Top - height, Width - 12, 22),
                };
                saveButton.Clicked += _cooker.SaveData;
            }

            public void RefreshColumn(BuildTarget[] targets, int selectedIndex)
            {
                RemoveButtons();

                if (targets != null)
                {
                    for (int i = 0; i < targets.Length; i++)
                    {
                        if (targets[i] == null)
                            return;
                        AddButton(targets[i].Name, i, selectedIndex,
                                  b => _cooker.SelectTarget((int)b.Tag),
                                  b => _cooker.RemoveTarget((int)b.Tag));
                    }
                }
            }
        }

        private struct QueueItem
        {
            public string PresetName;
            public BuildTarget Target;
            public BuildOptions Options;
        }

        private PresetsColumn _presets;
        private TargetsColumn _targets;
        private BuildTabProxy _buildTabProxy;
        private int _selectedPresetIndex = -1;
        private int _selectedTargetIndex = -1;
        private CustomEditorPresenter _targetSettings;
        private readonly Queue<QueueItem> _buildingQueue = new Queue<QueueItem>();
        private string _preBuildAction;
        private string _postBuildAction;
        private BuildPreset[] _data;
        private bool _isDataDirty, _exitOnBuildEnd, _lastBuildFailed;

        /// <summary>
        /// Initializes a new instance of the <see cref="GameCookerWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public GameCookerWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Game Cooker";

            var sections = new Tabs
            {
                Orientation = Orientation.Vertical,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                TabsSize = new Float2(120, 32),
                Parent = this
            };

            CreatePresetsTab(sections);
            CreateBuildTab(sections);

            GameCooker.Event += OnGameCookerEvent;

            sections.SelectedTabIndex = 1;
        }

        private void OnGameCookerEvent(GameCooker.EventType type)
        {
            if (type == GameCooker.EventType.BuildStarted)
            {
                // Execute pre-build action
                if (!string.IsNullOrEmpty(_preBuildAction))
                    ExecuteAction(_preBuildAction);
                _preBuildAction = null;
            }
            else if (type == GameCooker.EventType.BuildDone)
            {
                // Execute post-build action
                if (!string.IsNullOrEmpty(_postBuildAction))
                    ExecuteAction(_postBuildAction);
                _postBuildAction = null;
            }
            else if (type == GameCooker.EventType.BuildFailed)
            {
                _postBuildAction = null;
                _lastBuildFailed = true;
            }
        }

        private void ExecuteAction(string action)
        {
            string command = "echo off\ncd \"" + Globals.ProjectFolder.Replace('/', '\\') + "\"\necho on\n" + action;
            command = command.Replace("\n", "\r\n");

            // TODO: postprocess text using $(OutputPath) etc. macros
            // TODO: capture std out of the action (maybe call system() to execute it)

            try
            {
                var tmpBat = StringUtils.CombinePaths(Globals.TemporaryFolder, Guid.NewGuid().ToString("N") + ".bat");
                File.WriteAllText(tmpBat, command);
                var procSettings = new CreateProcessSettings
                {
                    FileName = tmpBat,
                    HiddenWindow = true,
                    WaitForEnd = true,
                };
                Platform.CreateProcess(ref procSettings);
                File.Delete(tmpBat);
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
                Debug.LogError("Failed to execute build action.");
            }
        }

        internal void ExitOnBuildQueueEnd()
        {
            _exitOnBuildEnd = true;
        }

        /// <summary>
        /// Returns true if can build for the given platform (both supported and available).
        /// </summary>
        /// <param name="platformType">The platform.</param>
        /// <returns>True if can build, otherwise false.</returns>
        public bool CanBuild(PlatformType platformType)
        {
            if (_buildTabProxy.PerPlatformOptions.TryGetValue(platformType, out var platform))
                return platform.IsAvailable && platform.IsSupported;
            return false;
        }

        /// <summary>
        /// Builds all the targets from the given preset.
        /// </summary>
        /// <param name="preset">The preset.</param>
        public void BuildAll(BuildPreset preset)
        {
            if (preset == null)
                throw new ArgumentNullException(nameof(preset));

            Editor.Log("Building all targets");
            foreach (var e in preset.Targets)
            {
                _buildingQueue.Enqueue(new QueueItem
                {
                    PresetName = preset.Name,
                    Target = e.DeepClone(),
                });
            }
        }

        /// <summary>
        /// Builds the target.
        /// </summary>
        /// <param name="preset">The preset.</param>
        /// <param name="target">The target.</param>
        public void Build(BuildPreset preset, BuildTarget target)
        {
            if (target == null)
                throw new ArgumentNullException(nameof(target));

            Editor.Log("Building target");
            _buildingQueue.Enqueue(new QueueItem
            {
                PresetName = preset.Name,
                Target = target.DeepClone(),
            });
        }

        /// <summary>
        /// Builds the target for this platform and runs it on this device.
        /// </summary>
        public void BuildAndRun()
        {
            Editor.Log("Building and running");
            GameCooker.GetCurrentPlatform(out var platform, out var buildPlatform, out var buildConfiguration);
            var numberOfClients = Editor.Options.Options.Interface.NumberOfGameClientsToLaunch;
            for (int i = 0; i < numberOfClients; i++)
            {
                var buildOptions = BuildOptions.AutoRun;
                if (i > 0)
                    buildOptions |= BuildOptions.NoCook;

                _buildingQueue.Enqueue(new QueueItem
                {
                    Target = new BuildTarget
                    {
                        Output = _buildTabProxy.PerPlatformOptions[platform].Output,
                        Platform = buildPlatform,
                        Mode = buildConfiguration,
                    },
                    Options = buildOptions,
                });
            }
        }

        /// <summary>
        /// Runs the cooked game for this platform on this device.
        /// </summary>
        public void RunCooked()
        {
            Editor.Log("Running cooked build");
            GameCooker.GetCurrentPlatform(out var platform, out var buildPlatform, out var buildConfiguration);
            var numberOfClients = Editor.Options.Options.Interface.NumberOfGameClientsToLaunch;
            for (int i = 0; i < numberOfClients; i++)
            {
                _buildingQueue.Enqueue(new QueueItem
                {
                    Target = new BuildTarget
                    {
                        Output = _buildTabProxy.PerPlatformOptions[platform].Output,
                        Platform = buildPlatform,
                        Mode = buildConfiguration,
                    },
                    Options = BuildOptions.AutoRun | BuildOptions.NoCook,
                });
            }
        }

        private void BuildTarget()
        {
            if (_data == null || _data.Length <= _selectedPresetIndex || _selectedPresetIndex == -1)
                return;
            if (_data[_selectedPresetIndex].Targets == null || _data[_selectedPresetIndex].Targets.Length <= _selectedTargetIndex)
                return;

            var preset = _data[_selectedPresetIndex];
            Build(preset, preset.Targets[_selectedTargetIndex]);
        }

        private void BuildAllTargets()
        {
            if (_data == null || _data.Length <= _selectedPresetIndex || _selectedPresetIndex == -1)
                return;
            if (_data[_selectedPresetIndex].Targets == null || _data[_selectedPresetIndex].Targets.Length == 0)
                return;

            BuildAll(_data[_selectedPresetIndex]);
        }

        private void AddPreset()
        {
            var count = _data?.Length ?? 0;
            var presets = new BuildPreset[count + 1];
            if (count > 0)
                Array.Copy(_data, presets, count);
            presets[count] = new BuildPreset
            {
                Name = "Preset " + (count + 1),
                Targets = new[]
                {
                    new BuildTarget
                    {
                        Name = "Windows 64bit",
                        Output = "Output\\Win64",
                        Platform = BuildPlatform.Windows64,
                        Mode = BuildConfiguration.Development,
                    },
                }
            };
            _data = presets;

            MarkAsEdited();
            RefreshColumns();
        }

        private void AddTarget()
        {
            if (_data == null || _data.Length <= _selectedPresetIndex)
                return;

            var count = _data[_selectedPresetIndex].Targets?.Length ?? 0;
            var targets = new BuildTarget[count + 1];
            if (count > 0)
                Array.Copy(_data[_selectedPresetIndex].Targets, targets, count);
            targets[count] = new BuildTarget
            {
                Name = "Windows 64bit",
                Output = "Output\\Win64",
                Platform = BuildPlatform.Windows64,
                Mode = BuildConfiguration.Development,
            };
            _data[_selectedPresetIndex].Targets = targets;

            MarkAsEdited();
            RefreshColumns();
        }

        private void SelectPreset(int index)
        {
            SelectTarget(index, 0);
        }

        private void SelectTarget(int index)
        {
            SelectTarget(_selectedPresetIndex, index);
        }

        private void RemovePreset(int index)
        {
            if (_data == null || _data.Length <= index)
                return;
            var presets = _data.ToList();
            presets.RemoveAt(index);
            _data = presets.ToArray();
            MarkAsEdited();

            if (presets.Count == 0)
            {
                SelectTarget(-1, -1);
            }
            else if (_selectedPresetIndex == index)
            {
                SelectTarget(0, 0);
            }
            else
            {
                RefreshColumns();
            }
        }

        private void RemoveTarget(int index)
        {
            if (_selectedPresetIndex == -1 || _data == null || _data.Length <= _selectedPresetIndex)
                return;

            var preset = _data[_selectedPresetIndex];
            var targets = preset.Targets.ToList();
            targets.RemoveAt(index);
            preset.Targets = targets.ToArray();
            MarkAsEdited();

            if (targets.Count == 0)
            {
                SelectTarget(_selectedPresetIndex, -1);
            }
            else if (_selectedPresetIndex == index)
            {
                SelectTarget(_selectedPresetIndex, 0);
            }
            else
            {
                RefreshColumns();
            }
        }

        private void RefreshColumns()
        {
            _presets.RefreshColumn(_data, _selectedPresetIndex);
            var presets = _data != null && _data.Length > _selectedPresetIndex && _selectedPresetIndex != -1 ? _data[_selectedPresetIndex].Targets : null;
            _targets.RefreshColumn(presets, _selectedTargetIndex);
        }

        private void SelectTarget(int presetIndex, int targetIndex)
        {
            object obj = null;
            if (presetIndex != -1 && targetIndex != -1 && _data != null && _data.Length > presetIndex)
            {
                var preset = _data[presetIndex];
                if (preset.Targets != null && preset.Targets.Length > targetIndex)
                    obj = preset.Targets[targetIndex];
            }

            _targetSettings.Select(obj);
            _selectedPresetIndex = presetIndex;
            _selectedTargetIndex = targetIndex;

            RefreshColumns();
        }

        private void CreatePresetsTab(Tabs sections)
        {
            var tab = sections.AddTab(new Tab("Presets"));

            _presets = new PresetsColumn(tab, this);
            _targets = new TargetsColumn(tab, this);
            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(140 * 2, 0, 0, 0),
                Parent = tab
            };

            _targetSettings = new CustomEditorPresenter(null);
            _targetSettings.Panel.Parent = panel;
            _targetSettings.Modified += MarkAsEdited;
        }

        private void MarkAsEdited()
        {
            if (!_isDataDirty)
            {
                _isDataDirty = true;
            }
        }

        private void ClearDirtyFlag()
        {
            if (_isDataDirty)
            {
                _isDataDirty = false;
            }
        }

        private void CreateBuildTab(Tabs sections)
        {
            var tab = sections.AddTab(new Tab("Build"));

            var platformSelector = new PlatformSelector
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                BackgroundColor = Style.Current.LightBackground,
                Parent = tab,
            };
            platformSelector.SizeChanged += OnPlatformSelectorSizeChanged;
            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, platformSelector.Offsets.Height, 0),
                Parent = tab,
            };

            var settings = new CustomEditorPresenter(null);
            settings.Panel.Parent = panel;
            _buildTabProxy = new BuildTabProxy(this, platformSelector);
            settings.Select(_buildTabProxy);
        }

        private void OnPlatformSelectorSizeChanged(Control platformSelector)
        {
            var panel = platformSelector.Parent.Children[platformSelector.IndexInParent + 1];
            panel.Offsets = new Margin(0, 0, platformSelector.Offsets.Height, 0);
        }

        /// <summary>
        /// Load the build presets from the settings.
        /// </summary>
        private void GatherData()
        {
            _data = null;

            var settings = GameSettings.Load<BuildSettings>();
            if (settings.Presets != null)
            {
                _data = new BuildPreset[settings.Presets.Length];
                for (int i = 0; i < _data.Length; i++)
                {
                    _data[i] = settings.Presets[i].DeepClone();
                }
            }

            ClearDirtyFlag();
            SelectTarget(0, 0);
        }

        /// <summary>
        /// Saves the build presets to the settings.
        /// </summary>
        private void SaveData()
        {
            if (_data == null)
                return;

            var settings = GameSettings.Load<BuildSettings>();
            settings.Presets = _data;
            GameSettings.Save(settings);

            ClearDirtyFlag();
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            GatherData();
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            // Building queue
            if (!GameCooker.IsRunning)
            {
                if (_buildingQueue.Count > 0)
                {
                    var item = _buildingQueue.Dequeue();
                    var target = item.Target;

                    _preBuildAction = target.PreBuildAction;
                    _postBuildAction = target.PostBuildAction;

                    bool failed = GameCooker.Build(target.Platform, target.Mode, target.Output, item.Options, target.CustomDefines, item.PresetName, target.Name);
                    if (failed && _exitOnBuildEnd)
                    {
                        _exitOnBuildEnd = false;
                        Engine.RequestExit(1);
                    }
                }
                else if (_exitOnBuildEnd)
                {
                    _exitOnBuildEnd = false;
                    Engine.RequestExit(_lastBuildFailed ? 1 : 0);
                }
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            GameCooker.Event -= OnGameCookerEvent;
            _buildTabProxy = null;

            base.OnDestroy();
        }
    }
}
