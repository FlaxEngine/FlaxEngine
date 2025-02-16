// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.IO;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Audio clip window allows to view and edit <see cref="AudioClip"/> asset.
    /// </summary>
    /// <seealso cref="AudioClip" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class AudioClipWindow : AssetEditorWindowBase<AudioClip>
    {
        private sealed class Preview : AudioClipPreview
        {
            public AudioSource Source;

            public override void Draw()
            {
                base.Draw();

                if (!Source || Source.State == AudioSource.States.Stopped)
                    return;
                var info = DataInfo;
                if (!HasData || info.NumSamples == 0)
                    return;
                var height = Height;
                var width = Width;

                // Draw current time
                var playPosition = Source.Time / info.Length * width;
                Render2D.DrawLine(new Float2(playPosition, 0), new Float2(playPosition, height), Color.White);

                // Draw current mouse pointer
                var mousePos = PointFromScreen(Input.MouseScreenPosition);
                if (mousePos.X > 0 && mousePos.Y > 0 && mousePos.X < width && mousePos.Y < height)
                {
                    Render2D.DrawLine(new Float2(mousePos.X, 0), new Float2(mousePos.X, height), Color.White.AlphaMultiplied(0.3f));
                }
            }

            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (base.OnMouseDown(location, button))
                    return true;

                if (button == MouseButton.Left && Source && Source.State != AudioSource.States.Stopped)
                {
                    var info = DataInfo;
                    Source.Time = location.X / Width * info.Length;
                }
                return false;
            }
        }

        /// <summary>
        /// The AudioClip properties proxy object.
        /// </summary>
        [CustomEditor(typeof(ProxyEditor))]
        private sealed class PropertiesProxy
        {
            private AudioClipWindow _window;

            [EditorOrder(1000), EditorDisplay("Import Settings", EditorDisplayAttribute.InlineStyle)]
            public AudioImportSettings ImportSettings = new AudioImportSettings();

            public sealed class ProxyEditor : GenericEditor
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var window = ((PropertiesProxy)Values[0])._window;
                    if (window == null)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }

                    // Audio properties
                    {
                        var audio = window.Asset;
                        AudioDataInfo info = audio.Info;
                        Editor.Internal_GetAudioClipMetadata(FlaxEngine.Object.GetUnmanagedPtr(audio), out int originalSize, out int importedSize);

                        var group = layout.Group("General");
                        group.Label("Format: " + audio.Format);
                        group.Label("Length: " + (Mathf.CeilToInt(audio.Length * 100.0f) / 100.0f) + "s");
                        group.Label(string.Format("{0}kHz, channels: {1}, bit depth: {2}", info.SampleRate / 1000, info.NumChannels, info.BitDepth));
                        group.Label("Original size: " + Utilities.Utils.FormatBytesCount((ulong)originalSize));
                        group.Label("Imported size: " + Utilities.Utils.FormatBytesCount((ulong)importedSize));
                        group.Label("Compression ratio: " + Mathf.CeilToInt((float)importedSize / originalSize * 100.0f) + "%");
                    }

                    base.Initialize(layout);

                    // Creates the import path UI
                    Utilities.Utils.CreateImportPathUI(layout, window.Item as BinaryAssetItem);

                    layout.Space(5);
                    var reimportButton = layout.Button("Reimport");
                    reimportButton.Button.Clicked += () => ((PropertiesProxy)Values[0]).Reimport();
                }
            }

            /// <summary>
            /// Gathers parameters from the specified AudioClip.
            /// </summary>
            /// <param name="window">The asset window.</param>
            public void OnLoad(AudioClipWindow window)
            {
                // Link
                _window = window;

                // Try to restore target asset AudioClip import options (useful for fast reimport)
                Editor.TryRestoreImportOptions(ref ImportSettings.Settings, window.Item.Path);

                // Prepare restore data
                PeekState();
            }

            /// <summary>
            /// Records the current state to restore it on DiscardChanges.
            /// </summary>
            public void PeekState()
            {
            }

            /// <summary>
            /// Reimports asset.
            /// </summary>
            public void Reimport()
            {
                if (_window?._previewSource != null)
                {
                    _window._previewSource.Stop();
                    _window.UpdateToolstrip();
                }
                Editor.Instance.ContentImporting.Reimport((BinaryAssetItem)_window.Item, ImportSettings, true);
            }

            /// <summary>
            /// On discard changes
            /// </summary>
            public void DiscardChanges()
            {
            }

            /// <summary>
            /// Clears temporary data.
            /// </summary>
            public void OnClean()
            {
                // Unlink
                _window = null;
            }
        }

        private readonly SplitPanel _split;
        private readonly Preview _preview;
        private readonly CustomEditorPresenter _propertiesEditor;
        private readonly ToolStripButton _playButton;
        private readonly ToolStripButton _pauseButton;
        private EditorScene _previewScene;
        private AudioSource _previewSource;

        private readonly PropertiesProxy _properties;
        private bool _isWaitingForLoad;

        /// <inheritdoc />
        public AudioClipWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Split Panel
            _split = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.7f,
                Parent = this
            };

            // Preview
            _preview = new Preview
            {
                DrawMode = AudioClipPreview.DrawModes.Fill,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _split.Panel1
            };

            // Properties editor
            _propertiesEditor = new CustomEditorPresenter(null);
            _propertiesEditor.Panel.Parent = _split.Panel2;
            _properties = new PropertiesProxy();
            _propertiesEditor.Select(_properties);

            // Toolstrip
            _toolstrip.AddButton(Editor.Icons.Import64, () => Editor.ContentImporting.Reimport((BinaryAssetItem)Item)).LinkTooltip("Reimport");
            _toolstrip.AddSeparator();
            _playButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Play64, OnPlay).LinkTooltip("Play/stop audio (F5)");
            _pauseButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Pause64, OnPause).LinkTooltip("Pause audio (F6)");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/audio/audio-clip.html")).LinkTooltip("See documentation to learn more");

            InputActions.Add(options => options.Play, OnPlay);
            InputActions.Add(options => options.Pause, OnPause);
        }

        private void OnPlay()
        {
            if (!_previewScene)
            {
                _previewScene = new EditorScene();
            }
            if (!_previewSource)
            {
                _previewSource = new AudioSource
                {
                    Parent = _previewScene,
                    AllowSpatialization = false,
                    Clip = _asset,
                };
                _preview.Source = _previewSource;
            }
            if (_previewSource.State == AudioSource.States.Playing)
                _previewSource.Stop();
            else
                _previewSource.Play();
            UpdateToolstrip();
        }

        private void OnPause()
        {
            if (_previewSource)
            {
                if (_previewSource.State == AudioSource.States.Playing)
                    _previewSource.Pause();
                else
                    _previewSource.Play();
            }
            UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _properties.OnClean();
            _preview.Asset = null;
            if (_previewSource)
                _previewSource.Clip = null;
            _isWaitingForLoad = false;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.Asset = _asset;
            _isWaitingForLoad = true;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Invalidate data
            _isWaitingForLoad = true;
        }

        /// <inheritdoc />
        protected override void OnClose()
        {
            // Discard unsaved changes
            _properties.DiscardChanges();

            base.OnClose();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_previewSource)
            {
                _preview.Source = null;
                _previewSource.Stop();
                FlaxEngine.Object.Destroy(_previewSource);
                _previewSource = null;
            }
            FlaxEngine.Object.Destroy(ref _previewScene);

            base.OnDestroy();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            base.UpdateToolstrip();

            _playButton.Icon = _previewSource && _previewSource.State == AudioSource.States.Playing ? Editor.Icons.Stop64 : Editor.Icons.Play64;
            _pauseButton.Enabled = _previewSource && _previewSource.State == AudioSource.States.Playing;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Check if need to load
            if (_isWaitingForLoad && _asset.IsLoaded)
            {
                _isWaitingForLoad = false;

                // Init properties and parameters proxy
                _properties.OnLoad(this);
                _propertiesEditor.BuildLayout();
                if (_previewSource)
                    _previewSource.Stop();
                _preview.RefreshPreview();

                // Setup
                ClearEditedFlag();
            }

            // Tick scene
            if (_previewScene)
            {
                _previewScene.Update();
                UpdateToolstrip();
            }
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            if (key == KeyboardKeys.Spacebar)
            {
                if (_previewSource?.State == AudioSource.States.Playing)
                    OnPause();
                else
                    OnPlay();
            }
            return false;
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            LayoutSerializeSplitter(writer, "Split", _split);
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            LayoutDeserializeSplitter(node, "Split", _split);
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split.SplitterValue = 0.7f;
        }
    }
}
