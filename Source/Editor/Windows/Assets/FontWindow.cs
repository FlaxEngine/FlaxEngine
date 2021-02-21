// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="FontAsset"/> asset.
    /// </summary>
    /// <seealso cref="FontAsset" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class FontAssetWindow : AssetEditorWindowBase<FontAsset>
    {
        /// <summary>
        /// The font asset properties proxy object.
        /// </summary>
        private sealed class PropertiesProxy
        {
            [DefaultValue(FontHinting.Default)]
            [EditorOrder(10), EditorDisplay("Properties"), Tooltip("The font hinting used when rendering characters.")]
            public FontHinting Hinting;

            [DefaultValue(true)]
            [EditorOrder(20), EditorDisplay("Properties"), Tooltip("Enables using anti-aliasing for font characters. Otherwise font will use monochrome data.")]
            public bool AntiAliasing;

            [DefaultValue(false)]
            [EditorOrder(30), EditorDisplay("Properties"), Tooltip("Enables artificial embolden effect.")]
            public bool Bold;

            [DefaultValue(false)]
            [EditorOrder(40), EditorDisplay("Properties"), Tooltip("Enables slant effect, emulating italic style.")]
            public bool Italic;

            public void Get(out FontOptions options)
            {
                options = new FontOptions
                {
                    Hinting = Hinting
                };
                if (AntiAliasing)
                    options.Flags |= FontFlags.AntiAliasing;
                if (Bold)
                    options.Flags |= FontFlags.Bold;
                if (Italic)
                    options.Flags |= FontFlags.Italic;
            }

            public void Set(ref FontOptions options)
            {
                Hinting = options.Hinting;
                AntiAliasing = (options.Flags & FontFlags.AntiAliasing) == FontFlags.AntiAliasing;
                Bold = (options.Flags & FontFlags.Bold) == FontFlags.Bold;
                Italic = (options.Flags & FontFlags.Italic) == FontFlags.Italic;
            }
        }

        private TextBox _inputText;
        private Label _textPreview;
        private CustomEditorPresenter _propertiesEditor;
        private PropertiesProxy _proxy;
        private ToolStripButton _saveButton;

        /// <inheritdoc />
        public FontAssetWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var panel1 = new SplitPanel(Orientation.Horizontal, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.7f,
                Parent = this
            };
            var panel2 = new SplitPanel(Orientation.Vertical, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                SplitterValue = 0.2f,
                Parent = panel1.Panel1
            };

            // Text preview
            _inputText = new TextBox(true, 0, 0)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Parent = panel2.Panel1
            };
            _inputText.TextChanged += OnTextChanged;
            _textPreview = new Label
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Margin = new Margin(4),
                Wrapping = TextWrapping.WrapWords,
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Parent = panel2.Panel2
            };

            // Font asset properties
            _propertiesEditor = new CustomEditorPresenter(null);
            _propertiesEditor.Panel.Parent = panel1.Panel2;
            _propertiesEditor.Modified += OnPropertyEdited;
            _proxy = new PropertiesProxy();
            _propertiesEditor.Select(_proxy);

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Save32, Save).LinkTooltip("Save");
        }

        private void OnTextChanged()
        {
            _textPreview.Text = _inputText.Text;
        }

        private void OnPropertyEdited()
        {
            MarkAsEdited();

            _proxy.Get(out var options);
            var assetOptions = Asset.Options;
            if (assetOptions != options)
            {
                Asset.Options = options;
                Asset.Invalidate();
            }
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _textPreview.Font = new FontReference();

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            Asset.WaitForLoaded();
            _textPreview.Font = new FontReference(Asset.CreateFont(30));
            _inputText.Text = string.Format("This is a sample text using font {0}.", Asset.FamilyName);
            var options = Asset.Options;
            _proxy.Set(ref options);

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        public override void Save()
        {
            // Check if don't need to push any new changes to the original asset
            if (!IsEdited)
                return;

            // Save asset
            if (Asset.Save())
            {
                Editor.LogError("Cannot save asset.");
                return;
            }

            ClearEditedFlag();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            base.OnDestroy();

            _inputText = null;
            _textPreview = null;
            _propertiesEditor = null;
            _proxy = null;
            _saveButton = null;
        }
    }
}
