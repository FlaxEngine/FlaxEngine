// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Timeline;
using FlaxEditor.Scripting;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="Animation"/> asset.
    /// </summary>
    /// <seealso cref="Animation" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class AnimationWindow : AssetEditorWindowBase<Animation>
    {
        private sealed class Preview : AnimationPreview
        {
            private readonly AnimationWindow _window;
            private AnimationGraph _animGraph;

            public Preview(AnimationWindow window)
            : base(true)
            {
                _window = window;
                ShowFloor = true;
            }

            public void SetModel(SkinnedModel model)
            {
                PreviewActor.SkinnedModel = model;
                PreviewActor.AnimationGraph = null;
                Object.Destroy(ref _animGraph);
                if (!model)
                    return;

                // Use virtual animation graph to playback the animation
                _animGraph = FlaxEngine.Content.CreateVirtualAsset<AnimationGraph>();
                _animGraph.InitAsAnimation(model, _window.Asset, true, true);
                PreviewActor.AnimationGraph = _animGraph;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                var animation = _window.Asset;
                if (animation == null || !animation.IsLoaded)
                {
                    Render2D.DrawText(style.FontLarge, "Loading...", new Rectangle(Float2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
                }
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                Object.Destroy(ref _animGraph);

                base.OnDestroy();
            }
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class PropertiesProxy
        {
            private AnimationWindow Window;
            private Animation Asset;
            private ModelImportSettings ImportSettings = new ModelImportSettings();
            private bool EnablePreviewModelCache = true;

            [EditorDisplay("Preview"), NoSerialize, AssetReference(true), Tooltip("The skinned model to preview the animation playback.")]
            public SkinnedModel PreviewModel
            {
                get => Window?._preview?.SkinnedModel;
                set
                {
                    if (Window == null || PreviewModel == value)
                        return;
                    if (Window._preview == null)
                    {
                        // Animation preview
                        Window._preview = new Preview(Window)
                        {
                            ViewportCamera = new FPSCamera(),
                            ScaleToFit = false,
                            AnchorPreset = AnchorPresets.StretchAll,
                            Offsets = Margin.Zero,
                        };
                    }

                    Window._preview.SetModel(value);
                    Window._timeline.Preview = value ? Window._preview : null;

                    if (Window._panel2 == null)
                    {
                        // Properties panel
                        Window._panel2 = new SplitPanel(Orientation.Vertical, ScrollBars.None, ScrollBars.Vertical)
                        {
                            AnchorPreset = AnchorPresets.StretchAll,
                            Offsets = Margin.Zero,
                            SplitterValue = 0.6f,
                        };
                        Window._preview.Parent = Window._panel2.Panel1;
                    }

                    // Show panel2 with preview and properties or just properties inside panel2 2nd part
                    if (value)
                    {
                        Window._panel2.Parent = Window._panel1.Panel2;
                        Window._propertiesPresenter.Panel.Parent = Window._panel2.Panel2;
                    }
                    else
                    {
                        Window._panel2.Parent = null;
                        Window._propertiesPresenter.Panel.Parent = Window._panel1.Panel2;
                    }

                    if (value)
                    {
                        // Focus model
                        value.WaitForLoaded(500);
                        Window._preview.ViewportCamera.SetArcBallView(Window._preview.PreviewActor.Sphere);
                    }

                    if (EnablePreviewModelCache)
                    {
                        var customDataName = Window.GetPreviewModelCacheName();
                        if (value)
                            Window.Editor.ProjectCache.SetCustomData(customDataName, value.ID.ToString());
                        else
                            Window.Editor.ProjectCache.RemoveCustomData(customDataName);
                    }
                }
            }

            public void OnLoad(AnimationWindow window)
            {
                // Link
                Window = window;
                Asset = window.Asset;
                EnablePreviewModelCache = true;

                // Try to restore target asset import options (useful for fast reimport)
                Editor.TryRestoreImportOptions(ref ImportSettings.Settings, window.Item.Path);
            }

            public void OnClean()
            {
                // Unlink
                EnablePreviewModelCache = false;
                PreviewModel = null;
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
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }

                    // General properties
                    {
                        var group = layout.Group("General");

                        var info = proxy.Asset.Info;
                        group.Label("Length: " + info.Length + "s");
                        group.Label("Frames: " + info.FramesCount);
                        group.Label("Channels: " + info.ChannelsCount);
                        group.Label("Keyframes: " + info.KeyframesCount);
                        group.Label("Memory Usage: " + Utilities.Utils.FormatBytesCount((ulong)info.MemoryUsage));
                    }

                    base.Initialize(layout);

                    // Ignore import settings GUI if the type is not animation. This removes the import UI if the animation asset was not created using an import.
                    if (proxy.ImportSettings.Settings.Type != FlaxEngine.Tools.ModelTool.ModelType.Animation)
                        return;

                    // Import Settings
                    {
                        var group = layout.Group("Import Settings");

                        var importSettingsField = typeof(PropertiesProxy).GetField("ImportSettings", BindingFlags.NonPublic | BindingFlags.Instance);
                        var importSettingsValues = new ValueContainer(new ScriptMemberInfo(importSettingsField)) { proxy.ImportSettings };
                        group.Object(importSettingsValues);

                        // Creates the import path UI
                        Utilities.Utils.CreateImportPathUI(layout, proxy.Window.Item as BinaryAssetItem);

                        layout.Space(5);
                        var reimportButton = layout.Button("Reimport");
                        reimportButton.Button.Clicked += () => ((PropertiesProxy)Values[0]).Reimport();
                    }
                }
            }
        }

        private CustomEditorPresenter _propertiesPresenter;
        private PropertiesProxy _properties;
        private SplitPanel _panel1;
        private SplitPanel _panel2;
        private Preview _preview;
        private AnimationTimeline _timeline;
        private Undo _undo;
        private ToolStripButton _saveButton;
        private ToolStripButton _undoButton;
        private ToolStripButton _redoButton;
        private bool _isWaitingForTimelineLoad;
        private SkinnedModel _initialPreviewModel;
        private float _initialPanel2Splitter = 0.6f;

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
            var inputOptions = Editor.Options.Options.Input;

            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            // Main panel
            _panel1 = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.Vertical)
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
                Parent = _panel1.Panel1,
                Enabled = false
            };
            _timeline.Modified += MarkAsEdited;
            _timeline.SetNoTracksText("Loading...");

            // Asset properties
            _propertiesPresenter = new CustomEditorPresenter(null);
            _propertiesPresenter.Panel.Parent = _panel1.Panel2;
            _properties = new PropertiesProxy();
            _propertiesPresenter.Select(_properties);

            // Toolstrip
            _saveButton = _toolstrip.AddButton(Editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);
            _toolstrip.AddSeparator();
            _undoButton = _toolstrip.AddButton(Editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip("Undo", ref inputOptions.Undo);
            _redoButton = _toolstrip.AddButton(Editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip("Redo", ref inputOptions.Redo);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/animation/animation/index.html")).LinkTooltip("See documentation to learn more");

            // Setup input actions
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
        }

        private void OnUndoRedo(IUndoAction action)
        {
            MarkAsEdited();
            UpdateToolstrip();
        }

        private string GetPreviewModelCacheName()
        {
            return _asset.ID + ".PreviewModel";
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _properties.OnLoad(this);
            _propertiesPresenter.BuildLayout();
            ClearEditedFlag();
            if (!_initialPreviewModel && 
                Editor.ProjectCache.TryGetCustomData(GetPreviewModelCacheName(), out string str) && 
                Guid.TryParse(str, out var id))
            {
                _initialPreviewModel = FlaxEngine.Content.LoadAsync<SkinnedModel>(id);
            }
            if (_initialPreviewModel)
            {
                _properties.PreviewModel = _initialPreviewModel;
                _panel2.SplitterValue = _initialPanel2Splitter;
                _initialPreviewModel = null;
            }

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
            _timeline.Preview = null;

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
                _timeline._id = _asset.ID;
                _timeline.Load(_asset);
                _undo.Clear();
                _timeline.Enabled = true;
                _timeline.SetNoTracksText(null);
                ClearEditedFlag();
            }
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            LayoutSerializeSplitter(writer, "TimelineSplitter", _timeline.Splitter);
            LayoutSerializeSplitter(writer, "Panel1Splitter", _panel1);
            if (_panel2 != null)
                LayoutSerializeSplitter(writer, "Panel2Splitter", _panel2);
            writer.WriteAttributeString("TimeShowMode", _timeline.TimeShowMode.ToString());
            writer.WriteAttributeString("ShowPreviewValues", _timeline.ShowPreviewValues.ToString());
            if (_properties.PreviewModel)
                writer.WriteAttributeString("PreviewModel", _properties.PreviewModel.ID.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            LayoutDeserializeSplitter(node, "TimelineSplitter", _timeline.Splitter);
            LayoutDeserializeSplitter(node, "Panel1Splitter", _panel1);
            if (float.TryParse(node.GetAttribute("Panel2Splitter"), CultureInfo.InvariantCulture, out float value1) && value1 > 0.01f && value1 < 0.99f)
                _initialPanel2Splitter = value1;
            if (Enum.TryParse(node.GetAttribute("TimeShowMode"), out Timeline.TimeShowModes value2))
                _timeline.TimeShowMode = value2;
            if (bool.TryParse(node.GetAttribute("ShowPreviewValues"), out bool value3))
                _timeline.ShowPreviewValues = value3;
            if (Guid.TryParse(node.GetAttribute("PreviewModel"), out Guid value4))
                _initialPreviewModel = FlaxEngine.Content.LoadAsync<SkinnedModel>(value4);
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

            _preview = null;
            _timeline = null;
            _propertiesPresenter = null;
            _properties = null;
            _panel1 = null;
            _panel2 = null;
            _saveButton = null;
            _undoButton = null;
            _redoButton = null;

            base.OnDestroy();
        }
    }
}
