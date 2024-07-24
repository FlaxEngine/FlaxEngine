// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Create;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Base class for all Json asset proxy objects used to manage <see cref="JsonAssetItem"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.AssetProxy" />
    public abstract class JsonAssetBaseProxy : AssetProxy
    {
    }

    /// <summary>
    /// Json assets proxy.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetBaseProxy" />
    public abstract class JsonAssetProxy : JsonAssetBaseProxy
    {
        /// <summary>
        /// The json files extension.
        /// </summary>
        public static readonly string Extension = "json";

        /// <inheritdoc />
        public override string Name => "Json Asset";

        /// <inheritdoc />
        public override string FileExtension => Extension;

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new JsonAssetWindow(editor, (JsonAssetItem)item);
        }

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is JsonAssetItem json && json.TypeName == TypeName;
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0xd14f67);

        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new JsonAssetItem(path, id, typeName);
        }
    }

    /// <summary>
    /// Generic Json asset creating handler. Allows to specify type of the archetype class to use for the asset data object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.Create.CreateFileEntry" />
    public class GenericJsonCreateEntry : CreateFileEntry
    {
        /// <summary>
        /// The create options.
        /// </summary>
        public class Options
        {
            /// <summary>
            /// The type of the archetype class to use for the asset data object to create.
            /// </summary>
            [CustomEditor(typeof(Editor))]
            [Tooltip("The type of the archetype class to use for the asset data object to create.")]
            public System.Type Type;

            private sealed class Editor : TypeEditor
            {
                /// <inheritdoc />
                internal override void Initialize(CustomEditorPresenter presenter, LayoutElementsContainer layout, ValueContainer values)
                {
                    base.Initialize(presenter, layout, values);

                    if (_element != null)
                    {
                        // Define the rule for the types that can be used to create a json data asset
                        _element.CustomControl.CheckValid += type =>
                        type.Type != null &&
                        type.IsClass &&
                        type.Type.IsVisible &&
                        !type.IsAbstract &&
                        !type.IsGenericType &&
                        type.Type.GetConstructor(Type.EmptyTypes) != null &&
                        !typeof(FlaxEngine.GUI.Control).IsAssignableFrom(type.Type) &&
                        !typeof(FlaxEngine.Object).IsAssignableFrom(type.Type);
                    }
                }
            }
        }

        private readonly Options _options = new Options();

        /// <inheritdoc />
        public override object Settings => _options;

        /// <summary>
        /// Initializes a new instance of the <see cref="SettingsCreateEntry"/> class.
        /// </summary>
        /// <param name="resultUrl">The result file url.</param>
        public GenericJsonCreateEntry(string resultUrl)
        : base("Json Asset", resultUrl)
        {
        }

        /// <inheritdoc />
        public override bool Create()
        {
            // Create settings asset object and serialize it to pure json asset
            var data = Activator.CreateInstance(_options.Type);
            return Editor.SaveJsonAsset(ResultUrl, data);
        }
    }

    /// <summary>
    /// Generic Json assets proxy (supports all json assets that don't have dedicated proxy).
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetBaseProxy" />
    [ContentContextMenu("New/Json Asset")]
    public class GenericJsonAssetProxy : JsonAssetProxy
    {
        /// <inheritdoc />
        public override string TypeName => typeof(JsonAsset).FullName;

        /// <inheritdoc />
        public override bool AcceptsAsset(string typeName, string path)
        {
            return path.EndsWith(FileExtension, StringComparison.OrdinalIgnoreCase);
        }

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is JsonAssetItem;
        }

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            Editor.Instance.ContentImporting.Create(new GenericJsonCreateEntry(outputPath));
        }
    }

    /// <summary>
    /// Content proxy for a json assets of the given type that can be spawned in the editor.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetProxy" />
    public class SpawnableJsonAssetProxy<T> : JsonAssetProxy where T : new()
    {
        /// <inheritdoc />
        public override string Name { get; } = Utilities.Utils.GetPropertyNameUI(typeof(T).Name);

        private SpriteHandle _thumbnail;

        public SpawnableJsonAssetProxy()
        {
            _thumbnail = SpriteHandle.Invalid;
        }
        
        public SpawnableJsonAssetProxy(SpriteHandle thumbnail)
        {
            _thumbnail = thumbnail;
        }

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            Editor.SaveJsonAsset(outputPath, new T());
        }
        
        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return _thumbnail.IsValid ? new JsonAssetItem(path, id, typeName, _thumbnail) : base.ConstructItem(path, typeName, ref id);
        }

        /// <inheritdoc />
        public override string TypeName { get; } = typeof(T).FullName;
    }
}
