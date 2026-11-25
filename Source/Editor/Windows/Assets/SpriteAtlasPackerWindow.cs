// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI.Drag;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using System.Collections.Generic;
using System.ComponentModel;

namespace FlaxEngine
{
    partial class SpriteAtlasPacker
    {
        partial struct PackOptions
        {
            /// <summary>
            /// If checked, atlas size won't be automatic but manually set to Size proeprty.
            /// </summary>
            [HideInEditor]
            [DefaultValue(false)]
            public bool UseCustomSize
            {
                get => Flags.HasFlag(PackFlags.UseCustomSize);
                set => SetFlag(PackFlags.UseCustomSize, value);
            }

            /// <summary>
            /// If checked, atlas will automatically pack sprites on load when any of the input texures gets modified.
            /// </summary>
            [DefaultValue(true)]
            public bool AutoPack
            {
                get => Flags.HasFlag(PackFlags.AutoPack);
                set => SetFlag(PackFlags.AutoPack, value);
            }

            /// <summary>
            /// If checked, atlas will contain generated mip maps chain for the texture.
            /// </summary>
            [DefaultValue(true)]
            public bool GenerateMipMaps
            {
                get => Flags.HasFlag(PackFlags.GenerateMipMaps);
                set => SetFlag(PackFlags.GenerateMipMaps, value);
            }

            private void SetFlag(PackFlags flag, bool value)
            {
                Flags = value ? (Flags | flag) : (Flags & ~flag);
            }
        }
    }
}

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="SpriteAtlasPacker.PackOptions"/>.
    /// </summary>
    [CustomEditor(typeof(SpriteAtlasPacker.PackOptions)), DefaultEditor]
    class SpriteAtlasPackerPackOptionsEditor : GenericEditor
    {
        /// <inheritdoc />
        protected override List<ItemInfo> GetItemsForType(ScriptType type)
        {
            // Show both fields and properties
            return GetItemsForType(type, true, true);
        }
    }
}

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Sprite Atlas window allows to view and edit <see cref="SpriteAtlasPacker"/> asset.
    /// </summary>
    /// <seealso cref="SpriteAtlasPacker" />
    /// <seealso cref="FlaxEditor.Windows.Assets.SpriteAtlasWindow" />
    public sealed class SpriteAtlasPackerWindow : SpriteAtlasWindow
    {
        private class DragAreaControl : ContainerControl
        {
            private DragAssets _dragAssets;

            public PackingPropertiesProxy.ProxyEditor Editor;

            public DragAreaControl()
            : base(0, 0, 120, 40)
            {
                AutoFocus = false;
            }

            public override void DrawSelf()
            {
                base.DrawSelf();

                var style = Style.Current;
                var area = new Rectangle(Float2.Zero, Size);
                Render2D.DrawText(style.FontSmall, "Drag textures here", area, style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
                if (IsDragOver && _dragAssets.HasValidDrag)
                {
                    Render2D.FillRectangle(area, style.Selection);
                    Render2D.DrawRectangle(area, style.SelectionBorder);
                }
            }

            private bool ValidateAsset(AssetItem assetItem)
            {
                return assetItem is TextureAssetItem;
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
            {
                var result = base.OnDragEnter(ref location, data);
                if (result != DragDropEffect.None)
                    return result;

                if (_dragAssets == null)
                    _dragAssets = new DragAssets(ValidateAsset);
                _dragAssets.OnDragEnter(data);
                return _dragAssets.Effect;
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
            {
                var result = base.OnDragMove(ref location, data);
                if (result != DragDropEffect.None || _dragAssets == null)
                    return result;
                return _dragAssets.Effect;
            }

            /// <inheritdoc />
            public override void OnDragLeave()
            {
                _dragAssets?.OnDragLeave();

                base.OnDragLeave();
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
            {
                var result = base.OnDragDrop(ref location, data);
                if (result != DragDropEffect.None)
                    return result;

                if (_dragAssets != null)
                {
                    if (_dragAssets.HasValidDrag)
                    {
                        // Add textures to list of items (with undo)
                        result = _dragAssets.Effect;
                        var proxy = (PackingPropertiesProxy)Editor.Values[0];
                        var textures = new List<SpriteAtlasPacker.PackTexture>((SpriteAtlasPacker.PackTexture[])proxy.Textures.Clone());
                        foreach (var e in _dragAssets.Objects)
                        {
                            if (e is TextureAssetItem textureItem)
                                textures.Add(new SpriteAtlasPacker.PackTexture { Texture = textureItem.ID });
                        }
                        Editor.ChildrenEditors[1].SetValue(textures.ToArray());
                    }
                    _dragAssets.OnDragDrop(null);
                }

                return result;
            }
        }

        /// <summary>
        /// The atlas packing properties proxy object.
        /// </summary>
        [CustomEditor(typeof(ProxyEditor))]
        private sealed class PackingPropertiesProxy : PropertiesProxyBase
        {
            [EditorOrder(0)]
            public SpriteAtlasPacker.PackOptions Options;

            [EditorOrder(1)]
            public SpriteAtlasPacker.PackTexture[] Textures;

            public sealed class ProxyEditor : GenericEditor
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (PackingPropertiesProxy)Values[0];
                    var atlas = (SpriteAtlas)proxy._window?.Asset;
                    if (atlas == null || !atlas.IsLoaded)
                    {
                        layout.LoadingLabel();
                        return;
                    }

                    base.Initialize(layout);

                    // Add area to quickly add texture via drag&drop
                    var dragArea = layout.CustomContainer<DragAreaControl>();
                    dragArea.CustomControl.Editor = this;
                }
            }

            public override void OnLoad(TextureBaseWindow window)
            {
                base.OnLoad(window);

                var atlas = (SpriteAtlasPacker)window.Asset;
                Options = atlas.Options;
                Textures = atlas.Textures;
                ((SpriteAtlasPackerWindow)_window).CacheState();
            }

            public override void OnClean()
            {
                Textures = null;
                base.OnClean();
            }
        }

        private class PackingTab : Tab
        {
            public PackingTab(SpriteAtlasPackerWindow window)
            : base("Packing", window, true, window._undo)
            {
                Proxy = new PackingPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private PackingTab _packingTab;
        private SpriteAtlasPacker.PackOptions _savedOptions;
        private SpriteAtlasPacker.PackTexture[] _savedTextures;

        /// <inheritdoc />
        public SpriteAtlasPackerWindow(Editor editor, AssetItem item)
        : base(editor, item, false)
        {
            // Hide not needed UI elements
            _tabs.RemoveTab("Import");
            _toolstrip.GetButton(editor.Icons.Import64).Dispose();

            // Items to pack editing
            _packingTab = _tabs.AddTab(new PackingTab(this));
            _packingTab.IndexInParent -= 2;
            _packingTab.Presenter.Modified += PackAtlas;
            _undo.UndoDone += OnUndo;
            _undo.RedoDone += OnUndo;
            _tabs.SelectedTab = _packingTab;
            _tabs.OnChildrenChanged();
        }

        /// <inheritdoc />
        public override void Save()
        {
            base.Save();

            _item.RefreshThumbnail();
            CacheState();
        }

        /// <inheritdoc />
        protected override void OnClose()
        {
            if (Asset && IsEdited)
            {
                // Discard unsaved changes
                var atlas = (SpriteAtlasPacker)Asset;
                atlas.Options = _savedOptions;
                atlas.Textures = _savedTextures;
                atlas.Pack();
            }

            _undo.Clear();

            base.OnClose();
        }

        private void CacheState()
        {
            var proxy = (PackingPropertiesProxy)_packingTab.Proxy;
            _savedOptions = proxy.Options;
            _savedTextures = (SpriteAtlasPacker.PackTexture[])proxy.Textures.Clone();
        }

        private void PackAtlas()
        {
            var atlas = (SpriteAtlasPacker)Asset;
            var proxy = (PackingPropertiesProxy)_packingTab.Proxy;
            atlas.Options = proxy.Options;
            atlas.Textures = proxy.Textures;
            atlas.Pack();
            MarkAsEdited();
            OnSpritesEdited();
            _isWaitingForTexture = true;
        }

        private void OnUndo(IUndoAction action)
        {
            PackAtlas();
        }
    }
}
