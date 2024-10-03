// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tabs;
using FlaxEngine;
using FlaxEngine.GUI;

#pragma warning disable 1591

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="ModelBase"/> asset.
    /// </summary>
    /// <seealso cref="Model" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public abstract class ModelBaseWindow<TAsset, TWindow> : AssetEditorWindowBase<TAsset>
    where TAsset : ModelBase
    where TWindow : ModelBaseWindow<TAsset, TWindow>
    {
        protected abstract class PropertiesProxyBase
        {
            [HideInEditor]
            public TWindow Window;

            [HideInEditor]
            public TAsset Asset;

            public virtual void OnLoad(TWindow window)
            {
                Window = window;
                Asset = window.Asset;
            }

            public virtual void OnSave()
            {
            }

            public virtual void OnClean()
            {
                Window = null;
                Asset = null;
            }
        }

        protected abstract class ProxyEditorBase : GenericEditor
        {
            internal override void RefreshInternal()
            {
                // Skip updates when model is not loaded
                var proxy = (PropertiesProxyBase)Values[0];
                if (proxy.Asset == null || !proxy.Asset.IsLoaded)
                    return;

                base.RefreshInternal();
            }
        }

        protected class Tab : GUI.Tabs.Tab
        {
            public CustomEditorPresenter Presenter;
            public PropertiesProxyBase Proxy;

            public Tab(string text, TWindow window, bool modifiesAsset = true)
            : base(text)
            {
                var scrollPanel = new Panel(ScrollBars.Vertical)
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                    Parent = this
                };

                Presenter = new CustomEditorPresenter(null);
                Presenter.Panel.Parent = scrollPanel;
                if (modifiesAsset)
                    Presenter.Modified += window.MarkAsEdited;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                Presenter.Deselect();
                Presenter = null;
                Proxy = null;

                base.OnDestroy();
            }
        }

        protected readonly SplitPanel _split;
        protected readonly Tabs _tabs;
        protected readonly ToolStripButton _saveButton;

        protected bool _refreshOnLODsLoaded;
        protected bool _skipEffectsGuiEvents;
        protected int _isolateIndex = -1;
        protected int _highlightIndex = -1;

        /// <inheritdoc />
        protected ModelBaseWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

            // Toolstrip
            _saveButton = _toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);

            // Split Panel
            _split = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.59f,
                Parent = this
            };

            // Properties tabs
            _tabs = new Tabs
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                TabsSize = new Float2(60, 20),
                TabsTextHorizontalAlignment = TextAlignment.Center,
                UseScroll = true,
                Parent = _split.Panel2
            };
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            foreach (var child in _tabs.Children)
            {
                if (child is Tab tab && tab.Proxy.Window != null)
                    tab.Proxy.OnClean();
            }

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            foreach (var child in _tabs.Children)
            {
                if (child is Tab tab)
                {
                    tab.Proxy.OnLoad((TWindow)this);
                    tab.Presenter.BuildLayout();
                }
            }
            ClearEditedFlag();

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Refresh the properties (will get new data in OnAssetLoaded)
            foreach (var child in _tabs.Children)
            {
                if (child is Tab tab)
                {
                    tab.Proxy.OnClean();
                    tab.Presenter.BuildLayout();
                }
            }
            ClearEditedFlag();
            _refreshOnLODsLoaded = true;

            base.OnItemReimported(item);
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
            _split.SplitterValue = 0.59f;
        }
    }
}
