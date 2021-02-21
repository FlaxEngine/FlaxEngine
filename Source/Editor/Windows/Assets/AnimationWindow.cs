// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Reflection;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Timeline;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="Animation"/> asset.
    /// </summary>
    /// <seealso cref="Animation" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class AnimationWindow : AssetEditorWindowBase<Animation>
    {
        [CustomEditor(typeof(ProxyEditor))]
        private sealed class PropertiesProxy
        {
            private AnimationWindow Window;
            private Animation Asset;
            private ModelImportSettings ImportSettings = new ModelImportSettings();

            public void OnLoad(AnimationWindow window)
            {
                // Link
                Window = window;
                Asset = window.Asset;

                // Try to restore target asset import options (useful for fast reimport)
                ModelImportSettings.TryRestore(ref ImportSettings, window.Item.Path);
            }

            public void OnClean()
            {
                // Unlink
                Window = null;
                Asset = null;
            }

            public void Reimport()
            {
                Editor.Instance.ContentImporting.Reimport((BinaryAssetItem)Window.Item, ImportSettings, true);
            }

            private class ProxyEditor : GenericEditor
            {
                /// <inheritdoc />
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (PropertiesProxy)Values[0];

                    if (proxy.Asset == null || !proxy.Asset.IsLoaded)
                    {
                        layout.Label("Loading...");
                        return;
                    }

                    base.Initialize(layout);

                    // General properties
                    {
                        var group = layout.Group("General");

                        var info = proxy.Asset.Info;
                        group.Label("Length: " + info.Length + "s");
                        group.Label("Frames: " + info.FramesCount);
                        group.Label("Channels: " + info.ChannelsCount);
                        group.Label("Keyframes: " + info.KeyframesCount);
                    }

                    // Import Settings
                    {
                        var group = layout.Group("Import Settings");

                        var importSettingsField = typeof(PropertiesProxy).GetField("ImportSettings", BindingFlags.NonPublic | BindingFlags.Instance);
                        var importSettingsValues = new ValueContainer(new ScriptMemberInfo(importSettingsField)) { proxy.ImportSettings };
                        group.Object(importSettingsValues);

                        layout.Space(5);
                        var reimportButton = group.Button("Reimport");
                        reimportButton.Button.Clicked += () => ((PropertiesProxy)Values[0]).Reimport();
                    }
                }
            }
        }

        private CustomEditorPresenter _propertiesPresenter;
        private PropertiesProxy _properties;
        private SplitPanel _panel;
        private AnimationTimeline _timeline;
        private Undo _undo;
        private ToolStripButton _saveButton;
        private ToolStripButton _undoButton;
        private ToolStripButton _redoButton;
        private bool _isWaitingForTimelineLoad;

        /// <summary>
        /// Gets the animation timeline editor.
        /// </summary>
        public AnimationTimeline Timeline => _timeline;

        /// <summary>
        /// Gets the undo history context for this window.
        /// </summary>
        public Undo Undo => _undo;

        /// <inheritdoc />
        public AnimationWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            // Main panel
            _panel = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                SplitterValue = 0.8f,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                Parent = this
            };

            // Timeline
            _timeline = new AnimationTimeline(_undo)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _panel.Panel1,
                Enabled = false
            };
            _timeline.Modified += MarkAsEdited;

            // Asset properties
            _propertiesPresenter = new CustomEditorPresenter(null);
            _propertiesPresenter.Panel.Parent = _panel.Panel2;
            _properties = new PropertiesProxy();
            _propertiesPresenter.Select(_properties);
            _propertiesPresenter.Modified += MarkAsEdited;

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Save32, Save).LinkTooltip("Save");
            _toolstrip.AddSeparator();
            _undoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Undo32, _undo.PerformUndo).LinkTooltip("Undo (Ctrl+Z)");
            _redoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Redo32, _undo.PerformRedo).LinkTooltip("Redo (Ctrl+Y)");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs32, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/animation/animation/index.html")).LinkTooltip("See documentation to learn more");

            // Setup input actions
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
        }

        private void OnUndoRedo(IUndoAction action)
        {
            MarkAsEdited();
            UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _properties.OnLoad(this);
            _propertiesPresenter.BuildLayout();
            ClearEditedFlag();

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            _timeline.Save(_asset);
            ClearEditedFlag();
            _item.RefreshThumbnail();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;
            _undoButton.Enabled = _undo.CanUndo;
            _redoButton.Enabled = _undo.CanRedo;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _isWaitingForTimelineLoad = false;
            _properties.OnClean();

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _isWaitingForTimelineLoad = true;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Refresh the properties (will get new data in OnAssetLoaded)
            _properties.OnClean();
            _propertiesPresenter.BuildLayout();
            ClearEditedFlag();

            // Reload timeline
            _timeline.Enabled = false;
            _isWaitingForTimelineLoad = true;

            base.OnItemReimported(item);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            if (_isWaitingForTimelineLoad && _asset.IsLoaded)
            {
                _isWaitingForTimelineLoad = false;
                _timeline.Load(_asset);
                _undo.Clear();
                _timeline.Enabled = true;
                ClearEditedFlag();
            }
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            writer.WriteAttributeString("TimelineSplitter", _timeline.Splitter.SplitterValue.ToString());
            writer.WriteAttributeString("TimeShowMode", _timeline.TimeShowMode.ToString());
            writer.WriteAttributeString("ShowPreviewValues", _timeline.ShowPreviewValues.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (float.TryParse(node.GetAttribute("TimelineSplitter"), out float value1))
                _timeline.Splitter.SplitterValue = value1;

            if (Enum.TryParse(node.GetAttribute("TimeShowMode"), out Timeline.TimeShowModes value2))
                _timeline.TimeShowMode = value2;

            if (bool.TryParse(node.GetAttribute("ShowPreviewValues"), out bool value3))
                _timeline.ShowPreviewValues = value3;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _timeline.Splitter.SplitterValue = 0.2f;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_undo != null)
            {
                _undo.Enabled = false;
                _undo.Clear();
                _undo = null;
            }

            _timeline = null;
            _propertiesPresenter = null;
            _properties = null;
            _panel = null;
            _saveButton = null;
            _undoButton = null;
            _redoButton = null;

            base.OnDestroy();
        }
    }
}
