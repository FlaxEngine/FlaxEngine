// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Text;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.GUI.Dialogs;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEditor.Windows.Profiler;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;
using DockPanel = FlaxEditor.GUI.Docking.DockPanel;
using DockState = FlaxEditor.GUI.Docking.DockState;
using FloatWindowDockPanel = FlaxEditor.GUI.Docking.FloatWindowDockPanel;
using Window = FlaxEngine.Window;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Manages Editor windows and popups.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class WindowsModule : EditorModule
    {
        private DateTime _lastLayoutSaveTime;
        private float _projectIconScreenshotTimeout = -1;
        private string _windowsLayoutPath;

        private struct WindowRestoreData
        {
            public string AssemblyName;
            public string TypeName;
        }

        private readonly List<WindowRestoreData> _restoreWindows = new List<WindowRestoreData>();

        /// <summary>
        /// The main editor window.
        /// </summary>
        public Window MainWindow { get; private set; }

        /// <summary>
        /// Occurs when main editor window is being closed.
        /// </summary>
        public event Action MainWindowClosing;

        /// <summary>
        /// The content window.
        /// </summary>
        public ContentWindow ContentWin;

        /// <summary>
        /// The edit game window.
        /// </summary>
        public EditGameWindow EditWin;

        /// <summary>
        /// The game window.
        /// </summary>
        public GameWindow GameWin;

        /// <summary>
        /// The properties window.
        /// </summary>
        public PropertiesWindow PropertiesWin;

        /// <summary>
        /// The scene tree window.
        /// </summary>
        public SceneTreeWindow SceneWin;

        /// <summary>
        /// The debug log window.
        /// </summary>
        public DebugLogWindow DebugLogWin;

        /// <summary>
        /// The output log window.
        /// </summary>
        public OutputLogWindow OutputLogWin;

        /// <summary>
        /// The toolbox window.
        /// </summary>
        public ToolboxWindow ToolboxWin;

        /// <summary>
        /// The graphics quality window.
        /// </summary>
        public GraphicsQualityWindow GraphicsQualityWin;

        /// <summary>
        /// The game cooker window.
        /// </summary>
        public GameCookerWindow GameCookerWin;

        /// <summary>
        /// The profiler window.
        /// </summary>
        public ProfilerWindow ProfilerWin;

        /// <summary>
        /// The editor options window.
        /// </summary>
        public EditorOptionsWindow EditorOptionsWin;

        /// <summary>
        /// The plugins manager window.
        /// </summary>
        public PluginsWindow PluginsWin;

        /// <summary>
        /// The Visual Script debugger window.
        /// </summary>
        public VisualScriptDebuggerWindow VisualScriptDebuggerWin;

        /// <summary>
        /// List with all created editor windows.
        /// </summary>
        public readonly List<EditorWindow> Windows = new List<EditorWindow>(32);

        /// <summary>
        /// Occurs when new window gets opened and added to the editor windows list.
        /// </summary>
        public event Action<EditorWindow> WindowAdded;

        /// <summary>
        /// Occurs when new window gets closed and removed from the editor windows list.
        /// </summary>
        public event Action<EditorWindow> WindowRemoved;

        internal WindowsModule(Editor editor)
        : base(editor)
        {
            InitOrder = -100;
        }

        /// <summary>
        /// Takes the screenshot of the current viewport.
        /// </summary>
        public void TakeScreenshot()
        {
            // Select task
            SceneRenderTask target = null;
            if (Editor.Windows.EditWin.IsSelected)
            {
                // Use editor window
                target = EditWin.Viewport.Task;
            }
            else
            {
                // Use game window
                GameWin.FocusOrShow();
            }

            // Fire screenshot taking
            Screenshot.Capture(target);
        }

        /// <summary>
        /// Updates the main window title.
        /// </summary>
        public void UpdateWindowTitle()
        {
            var mainWindow = MainWindow;
            if (mainWindow)
            {
                var projectPath = Globals.ProjectFolder.Replace('/', '\\');
                var platformBit = Platform.Is64BitApp ? "64" : "32";
                var title = string.Format("Flax Editor - \'{0}\' ({1}-bit)", projectPath, platformBit);
                mainWindow.Title = title;
            }
        }

        /// <summary>
        /// Flash main editor window to catch user attention
        /// </summary>
        public void FlashMainWindow()
        {
            MainWindow?.FlashWindow();
        }

        /// <summary>
        /// Finds the first window that is using given element to view/edit it.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>Editor window or null if cannot find any window.</returns>
        public EditorWindow FindEditor(ContentItem item)
        {
            for (int i = 0; i < Windows.Count; i++)
            {
                var win = Windows[i];
                if (win.IsEditingItem(item))
                {
                    return win;
                }
            }
            return null;
        }

        /// <summary>
        /// Closes all windows that are using given element to view/edit it.
        /// </summary>
        /// <param name="item">The item.</param>
        public void CloseAllEditors(ContentItem item)
        {
            for (int i = 0; i < Windows.Count; i++)
            {
                var win = Windows[i];
                if (win.IsEditingItem(item))
                {
                    win.Close();
                    i--;
                }
            }
        }

        /// <summary>
        /// Saves the current workspace layout.
        /// </summary>
        public void SaveCurrentLayout()
        {
            _lastLayoutSaveTime = DateTime.UtcNow;
            SaveLayout(_windowsLayoutPath);
        }

        /// <summary>
        /// Loads the default workspace layout for the current editor version.
        /// </summary>
        public void LoadDefaultLayout()
        {
            LoadLayout(StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/LayoutDefault.xml"));
        }

        /// <summary>
        /// Loads the layout from the file.
        /// </summary>
        /// <param name="path">The layout file path.</param>
        /// <returns>True if layout has been loaded otherwise if failed (e.g. missing file).</returns>
        public bool LoadLayout(string path)
        {
            if (Editor.IsHeadlessMode)
                return false;

            Editor.Log(string.Format("Loading editor windows layout from \'{0}\'", path));

            if (!File.Exists(path))
            {
                Editor.LogWarning("Cannot load windows layout. File is missing.");
                return false;
            }

            XmlDocument doc = new XmlDocument();
            var masterPanel = Editor.UI.MasterPanel;

            try
            {
                doc.Load(path);
                var root = doc["DockPanelLayout"];
                if (root == null)
                {
                    Editor.LogWarning("Invalid windows layout file.");
                    return false;
                }

                // Reset existing layout
                masterPanel.ResetLayout();

                // Get metadata
                int version = int.Parse(root.Attributes["Version"].Value, CultureInfo.InvariantCulture);
                var virtualDesktopBounds = Platform.VirtualDesktopBounds;
                var virtualDesktopSafeLeftCorner = virtualDesktopBounds.Location;
                var virtualDesktopSafeRightCorner = virtualDesktopBounds.BottomRight;

                switch (version)
                {
                case 4:
                {
                    // Main window info
                    if (MainWindow)
                    {
                        var mainWindowNode = root["MainWindow"];
                        Rectangle bounds = LoadBounds(mainWindowNode["Bounds"]);
                        bool isMaximized = bool.Parse(mainWindowNode.GetAttribute("IsMaximized"));

                        // Clamp position to match current desktop dimensions (if window was on desktop that is now inactive)
                        if (bounds.X < virtualDesktopSafeLeftCorner.X || bounds.Y < virtualDesktopSafeLeftCorner.Y || bounds.X > virtualDesktopSafeRightCorner.X || bounds.Y > virtualDesktopSafeRightCorner.Y)
                            bounds.Location = virtualDesktopSafeLeftCorner;

                        if (isMaximized)
                        {
                            if (MainWindow.IsMaximized)
                                MainWindow.Restore();
                            MainWindow.ClientPosition = bounds.Location;
                            MainWindow.Maximize();
                        }
                        else
                        {
                            if (Mathf.Min(bounds.Size.X, bounds.Size.Y) >= 1)
                            {
                                MainWindow.ClientBounds = bounds;
                            }
                            else
                            {
                                MainWindow.ClientPosition = bounds.Location;
                            }
                        }
                    }

                    // Load master panel structure
                    var masterPanelNode = root["MasterPanel"];
                    if (masterPanelNode != null)
                    {
                        LoadPanel(masterPanelNode, masterPanel);
                    }

                    // Load all floating windows structure
                    var floating = root.SelectNodes("Float");
                    if (floating != null)
                    {
                        foreach (XmlElement child in floating)
                        {
                            if (child == null)
                                continue;

                            // Get window properties
                            Rectangle bounds = LoadBounds(child["Bounds"]);

                            // Create window and floating dock panel
                            var window = FloatWindowDockPanel.CreateFloatWindow(MainWindow.GUI, bounds.Location, bounds.Size, WindowStartPosition.Manual, string.Empty);
                            var panel = new FloatWindowDockPanel(masterPanel, window.GUI);

                            // Load structure
                            LoadPanel(child, panel);

                            // Check if no child windows loaded (due to file errors or loading problems)
                            if (panel.TabsCount == 0 && panel.ChildPanelsCount == 0)
                            {
                                // Close empty window
                                Editor.LogWarning("Empty floating window inside layout.");
                                window.Close();
                            }
                            else
                            {
                                // Perform layout
                                var windowGUI = window.GUI;
                                windowGUI.UnlockChildrenRecursive();
                                windowGUI.PerformLayout();

                                // Show
                                window.Show();
                                window.Focus();

                                // Perform layout again
                                windowGUI.PerformLayout();
                            }
                        }
                    }

                    break;
                }

                default:
                {
                    Editor.LogWarning("Unsupported windows layout version");
                    return false;
                }
                }
            }
            catch (Exception ex)
            {
                Editor.LogWarning("Failed to load windows layout.");
                Editor.LogWarning(ex);
                return false;
            }

            return true;
        }

        private void SavePanel(XmlWriter writer, DockPanel panel)
        {
            writer.WriteAttributeString("SelectedTab", panel.SelectedTabIndex.ToString());

            for (int i = 0; i < panel.TabsCount; i++)
            {
                var win = panel.Tabs[i];
                writer.WriteStartElement("Window");

                writer.WriteAttributeString("Typename", win.SerializationTypename);

                if (win.UseLayoutData)
                {
                    writer.WriteStartElement("Data");
                    win.OnLayoutSerialize(writer);
                    writer.WriteEndElement();
                }

                writer.WriteEndElement();
            }

            for (int i = 0; i < panel.ChildPanelsCount; i++)
            {
                var p = panel.ChildPanels[i];

                // Skip empty panels
                if (p.TabsCount == 0)
                    continue;

                writer.WriteStartElement("Panel");

                DockState state = p.TryGetDockState(out float splitterValue);

                writer.WriteAttributeString("DockState", ((int)state).ToString());
                writer.WriteAttributeString("SplitterValue", splitterValue.ToString(CultureInfo.InvariantCulture));

                SavePanel(writer, p);

                writer.WriteEndElement();
            }
        }

        private void LoadPanel(XmlElement node, DockPanel panel)
        {
            int selectedTab = int.Parse(node.GetAttribute("SelectedTab"), CultureInfo.InvariantCulture);

            // Load docked windows
            var windows = node.SelectNodes("Window");
            if (windows != null)
            {
                foreach (XmlElement child in windows)
                {
                    if (child == null)
                        continue;

                    var typename = child.GetAttribute("Typename");
                    var window = GetWindow(typename);
                    if (window != null)
                    {
                        if (child.SelectSingleNode("Data") is XmlElement data)
                        {
                            window.OnLayoutDeserialize(data);
                        }
                        else
                        {
                            window.OnLayoutDeserialize();
                        }

                        window.Show(DockState.DockFill, panel);
                    }
                }
            }

            // Load child panels
            var panels = node.SelectNodes("Panel");
            if (panels != null)
            {
                foreach (XmlElement child in panels)
                {
                    if (child == null)
                        continue;

                    // Create child panel
                    DockState state = (DockState)int.Parse(child.GetAttribute("DockState"), CultureInfo.InvariantCulture);
                    float splitterValue = float.Parse(child.GetAttribute("SplitterValue"), CultureInfo.InvariantCulture);
                    var p = panel.CreateChildPanel(state, splitterValue);

                    LoadPanel(child, p);

                    // Check if panel has no docked window (due to loading problems or sth)
                    if (p.TabsCount == 0 && p.ChildPanelsCount == 0)
                    {
                        // Remove empty panel
                        Editor.LogWarning("Empty panel inside layout.");
                        p.RemoveIt();
                    }
                }
            }

            panel.SelectTab(selectedTab);
        }

        private static void SaveBounds(XmlWriter writer, Window win)
        {
            var bounds = win.ClientBounds;

            writer.WriteAttributeString("X", bounds.X.ToString(CultureInfo.InvariantCulture));
            writer.WriteAttributeString("Y", bounds.Y.ToString(CultureInfo.InvariantCulture));
            writer.WriteAttributeString("Width", bounds.Width.ToString(CultureInfo.InvariantCulture));
            writer.WriteAttributeString("Height", bounds.Height.ToString(CultureInfo.InvariantCulture));
        }

        private static Rectangle LoadBounds(XmlElement node)
        {
            float x = float.Parse(node.GetAttribute("X"), CultureInfo.InvariantCulture);
            float y = float.Parse(node.GetAttribute("Y"), CultureInfo.InvariantCulture);
            float width = float.Parse(node.GetAttribute("Width"), CultureInfo.InvariantCulture);
            float height = float.Parse(node.GetAttribute("Height"), CultureInfo.InvariantCulture);
            return new Rectangle(x, y, width, height);
        }

        private class LayoutNameDialog : Dialog
        {
            private TextBox _textbox;

            public LayoutNameDialog()
            : base("Enter Layout Name")
            {
                var name = new TextBox(false, 8, 8, 200)
                {
                    WatermarkText = "Enter layout slot name",
                    Parent = this,
                };
                _textbox = name;

                var okButton = new Button(name.Right - 50, name.Bottom + 4, 50)
                {
                    Text = "OK",
                    Parent = this,
                };
                okButton.Clicked += OnOk;

                var cancelButton = new Button(okButton.Left - 54, okButton.Y, 50)
                {
                    Text = "Cancel",
                    Parent = this,
                };
                cancelButton.Clicked += OnCancel;

                _dialogSize = okButton.BottomRight + new Vector2(8);
            }

            private void OnOk()
            {
                var name = _textbox.Text;
                if (name.Length == 0)
                {
                    MessageBox.Show("Cannot use the empty name.");
                    return;
                }
                if (Utilities.Utils.HasInvalidPathChar(name))
                {
                    MessageBox.Show("Cannot use this name. It contains one or more invalid characters.");
                    return;
                }

                Close(DialogResult.OK);

                var path = StringUtils.CombinePaths(Globals.ProjectCacheFolder, "Layout_" + name + ".xml");
                Editor.Instance.Windows.SaveLayout(path);
            }

            private void OnCancel()
            {
                Close(DialogResult.Cancel);
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                switch (key)
                {
                case KeyboardKeys.Escape:
                    OnCancel();
                    return true;
                case KeyboardKeys.Return:
                    OnOk();
                    return true;
                }

                return base.OnKeyDown(key);
            }
        }

        /// <summary>
        /// Asks user for the layout name and saves the current windows layout in the current project cache folder.
        /// </summary>
        public void SaveLayout()
        {
            if (Editor.IsHeadlessMode)
                return;

            new LayoutNameDialog().Show();
        }

        /// <summary>
        /// Saves the layout to the file.
        /// </summary>
        /// <param name="path">The layout file path.</param>
        public void SaveLayout(string path)
        {
            if (Editor.IsHeadlessMode)
                return;

            //Editor.Log(string.Format("Saving editor windows layout to \'{0}\'", path));

            var settings = new XmlWriterSettings
            {
                Indent = true,
                IndentChars = "\t",
                Encoding = Encoding.UTF8,
                OmitXmlDeclaration = true,
            };

            var masterPanel = Editor.UI.MasterPanel;
            if (masterPanel == null)
                return;

            using (XmlWriter writer = XmlWriter.Create(path, settings))
            {
                writer.WriteStartDocument();
                writer.WriteStartElement("DockPanelLayout");

                // Metadata
                writer.WriteAttributeString("Version", "4");

                // Main window info
                if (MainWindow)
                {
                    writer.WriteStartElement("MainWindow");
                    writer.WriteAttributeString("IsMaximized", MainWindow.IsMaximized.ToString());

                    writer.WriteStartElement("Bounds");
                    SaveBounds(writer, MainWindow);
                    writer.WriteEndElement();

                    writer.WriteEndElement();
                }

                // Master panel structure
                writer.WriteStartElement("MasterPanel");
                SavePanel(writer, masterPanel);
                writer.WriteEndElement();

                // Save all floating windows structure
                for (int i = 0; i < masterPanel.FloatingPanels.Count; i++)
                {
                    var panel = masterPanel.FloatingPanels[i];
                    var window = panel.Window;

                    if (window == null)
                    {
                        Editor.LogWarning("Floating panel has missing window");
                        continue;
                    }

                    writer.WriteStartElement("Float");

                    SavePanel(writer, panel);

                    writer.WriteStartElement("Bounds");
                    SaveBounds(writer, window.Window);
                    writer.WriteEndElement();

                    writer.WriteEndElement();
                }

                writer.WriteEndElement();
                writer.WriteEndDocument();
            }
        }

        /// <summary>
        /// Gets <see cref="EditorWindow"/> that is represented by the given serialized typename. Used to restore workspace layout.
        /// </summary>
        /// <param name="typename">The typename.</param>
        /// <returns>The window or null if failed.</returns>
        private EditorWindow GetWindow(string typename)
        {
            // Try use already opened window
            for (int i = 0; i < Windows.Count; i++)
            {
                if (string.Equals(Windows[i].SerializationTypename, typename, StringComparison.OrdinalIgnoreCase))
                {
                    return Windows[i];
                }
            }

            // Check if it's an asset ID
            if (Guid.TryParse(typename, out Guid id))
            {
                var el = Editor.ContentDatabase.Find(id);
                if (el != null)
                {
                    // Open asset
                    return Editor.ContentEditing.Open(el, true);
                }
            }

            return null;
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            Assert.IsNull(MainWindow);

            _windowsLayoutPath = StringUtils.CombinePaths(Globals.ProjectCacheFolder, "WindowsLayout.xml");

            // Create main window
            var dpiScale = Platform.DpiScale;
            var settings = CreateWindowSettings.Default;
            settings.Title = "Flax Editor";
            settings.Size = Platform.DesktopSize;
            settings.StartPosition = WindowStartPosition.CenterScreen;
            settings.ShowAfterFirstPaint = true;

#if PLATFORM_WINDOWS
            if (!Editor.Instance.Options.Options.Interface.UseNativeWindowSystem)
#endif
            {
                settings.HasBorder = false;
#if PLATFORM_WINDOWS
                // Skip OS sizing frame and implement it using LeftButtonHit
                settings.HasSizingFrame = false;
#endif
            }

            MainWindow = Platform.CreateWindow(ref settings);

            if (MainWindow == null)
            {
                // Error
                Editor.LogError("Failed to create editor main window!");
                return;
            }
            UpdateWindowTitle();

            // Link for main window events
            MainWindow.Closing += MainWindow_OnClosing;
            MainWindow.Closed += MainWindow_OnClosed;

            // Create default editor windows
            ContentWin = new ContentWindow(Editor);
            EditWin = new EditGameWindow(Editor);
            GameWin = new GameWindow(Editor);
            PropertiesWin = new PropertiesWindow(Editor);
            SceneWin = new SceneTreeWindow(Editor);
            DebugLogWin = new DebugLogWindow(Editor);
            OutputLogWin = new OutputLogWindow(Editor);
            ToolboxWin = new ToolboxWindow(Editor);
            GraphicsQualityWin = new GraphicsQualityWindow(Editor);
            GameCookerWin = new GameCookerWindow(Editor);
            ProfilerWin = new ProfilerWindow(Editor);
            EditorOptionsWin = new EditorOptionsWindow(Editor);
            PluginsWin = new PluginsWindow(Editor);
            VisualScriptDebuggerWin = new VisualScriptDebuggerWindow(Editor);

            // Bind events
            Level.SceneSaveError += OnSceneSaveError;
            Level.SceneLoaded += OnSceneLoaded;
            Level.SceneLoadError += OnSceneLoadError;
            Level.SceneLoading += OnSceneLoading;
            Level.SceneSaved += OnSceneSaved;
            Level.SceneSaving += OnSceneSaving;
            Level.SceneUnloaded += OnSceneUnloaded;
            Level.SceneUnloading += OnSceneUnloading;
            ScriptsBuilder.ScriptsReloadEnd += OnScriptsReloadEnd;
            Editor.StateMachine.StateChanged += OnEditorStateChanged;
        }

        internal void AddToRestore(CustomEditorWindow win)
        {
            var type = win.GetType();

            // Validate if can restore type
            var constructor = type.GetConstructor(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, null, Type.EmptyTypes, null);
            if (constructor == null || type.IsGenericType)
                return;

            WindowRestoreData winData;
            winData.AssemblyName = type.Assembly.GetName().Name;
            winData.TypeName = type.FullName;
            // TODO: cache and restore docking info
            _restoreWindows.Add(winData);
        }

        private void OnScriptsReloadEnd()
        {
            for (int i = 0; i < _restoreWindows.Count; i++)
            {
                var winData = _restoreWindows[i];

                try
                {
                    var assembly = Utils.GetAssemblyByName(winData.AssemblyName);
                    if (assembly != null)
                    {
                        var type = assembly.GetType(winData.TypeName);
                        if (type != null)
                        {
                            var win = (CustomEditorWindow)Activator.CreateInstance(type);
                            win.Show();
                        }
                    }
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(ex);
                    Editor.LogWarning(string.Format("Failed to restore window {0} (assembly: {1})", winData.TypeName, winData.AssemblyName));
                }
            }
            _restoreWindows.Clear();
        }

        private void MainWindow_OnClosing(ClosingReason reason, ref bool cancel)
        {
            Editor.Log("Main window is closing, reason: " + reason);

            if (Editor.StateMachine.IsPlayMode)
            {
                // Cancel closing but leave the play mode
                cancel = true;
                Editor.Log("Skip closing editor and leave the play mode");
                Editor.Simulation.RequestStopPlay();
                return;
            }

            SaveCurrentLayout();

            // Block closing only on user events
            if (reason == ClosingReason.User)
            {
                // Check if cancel action or save scene before exit
                if (Editor.Scene.CheckSaveBeforeClose())
                {
                    // Cancel
                    cancel = true;
                    return;
                }

                // Close all asset editor windows
                for (int i = 0; i < Windows.Count; i++)
                {
                    if (Windows[i] is AssetEditorWindow assetEditorWindow)
                    {
                        if (assetEditorWindow.Close(ClosingReason.User))
                        {
                            // Cancel
                            cancel = true;
                            return;
                        }

                        // Remove it
                        OnWindowRemove(assetEditorWindow);
                        i--;
                    }
                }
            }

            MainWindowClosing?.Invoke();
        }

        private void MainWindow_OnClosed()
        {
            Editor.Log("Main window is closed");
            MainWindow = null;

            // Capture project icon screenshot (not in play mode and if editor was used for some time)
            if (!Editor.StateMachine.IsPlayMode && Time.TimeSinceStartup >= 5.0f)
            {
                Editor.Log("Capture project icon screenshot");
                _projectIconScreenshotTimeout = Time.TimeSinceStartup + 0.8f; // wait 800ms for a screenshot task
                EditWin.Viewport.SaveProjectIcon();
            }
            else
            {
                // Close editor
                Engine.RequestExit();
            }
        }

        internal void OnWindowAdd(EditorWindow window)
        {
            Windows.Add(window);
            WindowAdded?.Invoke(window);
        }

        internal void OnWindowRemove(EditorWindow window)
        {
            Windows.Remove(window);
            WindowRemoved?.Invoke(window);
        }

        /// <inheritdoc />
        public override void OnEndInit()
        {
            UpdateWindowTitle();

            // Initialize windows
            for (int i = 0; i < Windows.Count; i++)
            {
                try
                {
                    Windows[i].OnInit();
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(ex);
                    Editor.LogError("Failed to init window " + Windows[i]);
                }
            }

            // Load current workspace layout
            if (!LoadLayout(_windowsLayoutPath))
                LoadDefaultLayout();

            // Clear timer flag
            _lastLayoutSaveTime = DateTime.UtcNow;
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            Profiler.BeginEvent("WindowsModule.Update");

            // Auto save workspace layout every few seconds
            var now = DateTime.UtcNow;
            if (_lastLayoutSaveTime.Ticks > 10 && now - _lastLayoutSaveTime >= TimeSpan.FromSeconds(10))
            {
                Profiler.BeginEvent("Save Layout");
                SaveCurrentLayout();
                Profiler.EndEvent();
            }

            // Auto close on project icon saving end
            if (_projectIconScreenshotTimeout > 0 && Time.TimeSinceStartup > _projectIconScreenshotTimeout)
            {
                Editor.Log("Closing Editor after project icon screenshot");
                EditWin.Viewport.SaveProjectIconEnd();
                Engine.RequestExit();
            }

            // Update editor windows
            for (int i = 0; i < Windows.Count; i++)
            {
                Windows[i].OnUpdate();
            }

            Profiler.EndEvent();
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            // Unbind events
            Level.SceneSaveError -= OnSceneSaveError;
            Level.SceneLoaded -= OnSceneLoaded;
            Level.SceneLoadError -= OnSceneLoadError;
            Level.SceneLoading -= OnSceneLoading;
            Level.SceneSaved -= OnSceneSaved;
            Level.SceneSaving -= OnSceneSaving;
            Level.SceneUnloaded -= OnSceneUnloaded;
            Level.SceneUnloading -= OnSceneUnloading;
            ScriptsBuilder.ScriptsReloadEnd -= OnScriptsReloadEnd;
            Editor.StateMachine.StateChanged -= OnEditorStateChanged;

            // Close main window
            MainWindow?.Close(ClosingReason.EngineExit);
            MainWindow = null;

            // Close all windows
            var windows = Windows.ToArray();
            for (int i = 0; i < windows.Length; i++)
            {
                if (windows[i] != null)
                    windows[i].Close(ClosingReason.EngineExit);
            }
        }

        #region Window Events

        private void OnEditorStateChanged()
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnEditorStateChanged();
        }

        private void OnSceneSaveError(Scene scene, Guid sceneId)
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnSceneSaveError(scene, sceneId);
        }

        private void OnSceneLoaded(Scene scene, Guid sceneId)
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnSceneLoaded(scene, sceneId);
        }

        private void OnSceneLoadError(Scene scene, Guid sceneId)
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnSceneLoadError(scene, sceneId);
        }

        private void OnSceneLoading(Scene scene, Guid sceneId)
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnSceneLoading(scene, sceneId);
        }

        private void OnSceneSaved(Scene scene, Guid sceneId)
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnSceneSaved(scene, sceneId);
        }

        private void OnSceneSaving(Scene scene, Guid sceneId)
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnSceneSaving(scene, sceneId);
        }

        private void OnSceneUnloaded(Scene scene, Guid sceneId)
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnSceneUnloaded(scene, sceneId);
        }

        private void OnSceneUnloading(Scene scene, Guid sceneId)
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnSceneUnloading(scene, sceneId);
        }

        /// <inheritdoc />
        public override void OnPlayBeginning()
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnPlayBeginning();
        }

        /// <inheritdoc />
        public override void OnPlayBegin()
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnPlayBegin();
        }

        /// <inheritdoc />
        public override void OnPlayEnd()
        {
            for (int i = 0; i < Windows.Count; i++)
                Windows[i].OnPlayEnd();
        }

        #endregion
    }
}
