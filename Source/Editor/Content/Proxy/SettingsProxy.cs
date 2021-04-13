// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Create;
using FlaxEditor.Content.Settings;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content proxy for json settings assets (e.g <see cref="GameSettings"/> or <see cref="TimeSettings"/>).
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetProxy" />
    public sealed class SettingsProxy : JsonAssetProxy
    {
        private readonly Type _type;

        /// <summary>
        /// Initializes a new instance of the <see cref="SettingsProxy"/> class.
        /// </summary>
        /// <param name="type">The settings asset type (must be subclass of SettingsBase type).</param>
        public SettingsProxy(Type type)
        {
            _type = type;
            TypeName = type.FullName;
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
            switch (typeName.Substring(typeName.LastIndexOf('.')+1))
            {
                case "GameSettings":            return new GameSettingsItem(path, id, typeName);
                case "AndroidPlatformSettings": return new AndroidPlatformSettingsItem(path, id, typeName);
                case "AudioSettings":           return new AudioSettingsItem(path, id, typeName);
                case "BuildSettings":           return new BuildSettingsItem(path, id, typeName);
                case "GraphicsSettings":        return new GraphicsSettingsItem(path, id, typeName);
                case "InputSettings":           return new InputSettingsItem(path, id, typeName);
                case "LayersAndTagsSettings":   return new LayersAndTagsSettingsItem(path, id, typeName);
                case "LinuxPlatformSettings":   return new LinuxPlatformSettingsItem(path, id, typeName);
                case "NavigationSettings":      return new NavigationSettingsItem(path, id, typeName);
                case "PhysicsSettings":         return new PhysicsSettingsItem(path, id, typeName);
                case "TimeSettings":            return new TimeSettingsItem(path, id, typeName);
                case "UWPPlatformSettings":     return new UWPPlatformSettingsItem(path, id, typeName);
                case "WindowsPlatformSettings": return new WindowsPlatformSettingsItem(path, id, typeName);
            }
            return base.ConstructItem(path, typeName, ref id);
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
