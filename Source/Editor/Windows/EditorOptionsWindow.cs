// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using FlaxEngine.Utilities;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Editor window for changing the options.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public sealed class EditorOptionsWindow : EditorWindow
    {
        private bool _isDataDirty;
        private Tabs _tabs;
        private EditorOptions _options;
        private ToolStripButton _saveButton;
        private readonly List<Tab> _customTabs = new List<Tab>();

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorOptionsWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public EditorOptionsWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Editor Options";

            var toolstrip = new ToolStrip
            {
                Parent = this
            };
            _saveButton = (ToolStripButton)toolstrip.AddButton(editor.Icons.Save64, SaveData).LinkTooltip("Save");
            _saveButton.Enabled = false;

            _tabs = new Tabs
            {
                Orientation = Orientation.Vertical,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, toolstrip.Bottom, 0),
                TabsSize = new Vector2(120, 32),
                UseScroll = true,
                Parent = this
            };

            CreateTab("General", () => _options.General);
            CreateTab("Interface", () => _options.Interface);
            CreateTab("Input", () => _options.Input);
            CreateTab("Viewport", () => _options.Viewport);
            CreateTab("Visual", () => _options.Visual);
            CreateTab("Source Code", () => _options.SourceCode);
            CreateTab("Theme", () => _options.Theme);

            _tabs.SelectedTabIndex = 0;
        }

        private Tab CreateTab(string name, Func<object> getValue)
        {
            var tab = _tabs.AddTab(new Tab(name));

            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = tab
            };

            var settings = new CustomEditorPresenter(null);
            settings.Panel.Parent = panel;
            settings.Panel.Tag = getValue;
            settings.Modified += MarkAsEdited;

            return tab;
        }

        private void MarkAsEdited()
        {
            if (!_isDataDirty)
            {
                _saveButton.Enabled = true;
                _isDataDirty = true;
            }
        }

        private void ClearDirtyFlag()
        {
            if (_isDataDirty)
            {
                _saveButton.Enabled = false;
                _isDataDirty = false;
            }
        }

        /// <summary>
        /// Load the editor options data.
        /// </summary>
        private void GatherData()
        {
            // Clone options (edit cloned version, not the current ones)
            _options = Editor.Options.Options.DeepClone();

            // Refresh tabs
            foreach (var c in _tabs.Children)
            {
                if (c is Tab tab)
                {
                    var panel = tab.GetChild<Panel>();
                    var settingsPanel = panel.GetChild<CustomEditorPresenter.PresenterPanel>();
                    var getValue = (Func<object>)settingsPanel.Tag;
                    var settings = settingsPanel.Presenter;
                    settings.Select(getValue());
                }
            }

            ClearDirtyFlag();
        }

        /// <summary>
        /// Saves the editor options data.
        /// </summary>
        private void SaveData()
        {
            if (_options == null || !_isDataDirty)
                return;

            // Flush custom settings
            foreach (var tab in _customTabs)
            {
                var name = tab.Text;

                var panel = tab.GetChild<Panel>();
                var settingsPanel = panel.GetChild<CustomEditorPresenter.PresenterPanel>();
                var settings = settingsPanel.Presenter;

                _options.CustomSettings[name] = JsonSerializer.Serialize(settings.Selection[0], typeof(object));
            }

            Editor.Options.Apply(_options);

            ClearDirtyFlag();
        }

        private void SetupCustomTabs()
        {
            // Remove old tabs
            foreach (var tab in _customTabs)
            {
                var panel = tab.GetChild<Panel>();
                var settingsPanel = panel.GetChild<CustomEditorPresenter.PresenterPanel>();
                var settings = settingsPanel.Presenter;
                settings.Deselect();

                tab.Dispose();
            }
            _customTabs.Clear();

            // Add new tabs
            foreach (var e in Editor.Options.CustomSettings)
            {
                var name = e.Key;

                // Ensure to have options object for that settings type
                if (!_options.CustomSettings.ContainsKey(name))
                    _options.CustomSettings.Add(name, JsonSerializer.Serialize(e.Value(), typeof(object)));

                // Create tab
                var tab = CreateTab(name, () => JsonSerializer.Deserialize(_options.CustomSettings[name]));
                tab.UnlockChildrenRecursive();
                _customTabs.Add(tab);

                // Update value
                var settingsPanel = tab.GetChild<Panel>().GetChild<CustomEditorPresenter.PresenterPanel>();
                try
                {
                    var value = JsonSerializer.Deserialize(_options.CustomSettings[name]);
                    settingsPanel.Presenter.Select(value);
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(ex);
                    _customTabs.Remove(tab);
                    tab.Dispose();
                }
            }
            if (_customTabs.Count != 0)
                _tabs.PerformLayout();
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            // Add custom settings tabs
            SetupCustomTabs();

            // Register for custom settings changes
            Editor.Options.CustomSettingsChanged += SetupCustomTabs;

            // Update UI
            GatherData();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _customTabs.Clear();
            _tabs = null;
            _saveButton = null;

            base.OnDestroy();
        }
    }
}
