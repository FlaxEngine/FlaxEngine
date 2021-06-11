// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Create;
using FlaxEditor.Content.Settings;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content proxy for json settings assets (e.g <see cref="GameSettings"/> or <see cref="TimeSettings"/>).
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetProxy" />
    public sealed class SettingsProxy : JsonAssetProxy
    {
        private readonly Type _type;
        private readonly SpriteHandle _thumbnail;

        /// <summary>
        /// Initializes a new instance of the <see cref="SettingsProxy"/> class.
        /// </summary>
        /// <param name="type">The settings asset type (must be subclass of SettingsBase type).</param>
        /// <param name="thumbnail">Asset icon.</param>
        public SettingsProxy(Type type, SpriteHandle thumbnail)
        {
            _type = type;
            TypeName = type.FullName;
            _thumbnail = thumbnail;
        }

        /// <inheritdoc />
        public override string Name => "Settings";
        //public override string Name { get; } = CustomEditors.CustomEditorsUtil.GetPropertyNameUI(_type.Name);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            // Use proxy only for GameSettings for creating
            if (_type != typeof(GameSettings))
                return false;

            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            Editor.Instance.ContentImporting.Create(new SettingsCreateEntry(outputPath));
        }

        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new JsonAssetItem(path, id, typeName, _thumbnail);
        }

        /// <inheritdoc />
        public override bool IsProxyFor<T>()
        {
            return typeof(T) == _type;
        }

        /// <inheritdoc />
        public override string TypeName { get; }
    }
}
