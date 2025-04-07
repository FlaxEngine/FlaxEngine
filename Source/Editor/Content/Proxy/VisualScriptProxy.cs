// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content.Create;
using FlaxEditor.Scripting;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.Json;
using FlaxEngine.Utilities;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="VisualScript"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    [ContentContextMenu("New/Visual Script")]
    public class VisualScriptProxy : BinaryAssetProxy, IScriptTypesContainer
    {
        internal VisualScriptProxy()
        {
            TypeUtils.CustomTypes.Add(this);
        }

        /// <inheritdoc />
        public override string Name => "Visual Script";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new VisualScriptWindow(editor, item as VisualScriptItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0xf5cb42);

        /// <inheritdoc />
        public override Type AssetType => typeof(VisualScript);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            Editor.Instance.ContentImporting.Create(new VisualScriptCreateEntry(outputPath));
        }

        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new VisualScriptItem(path, ref id, typeName, AssetType);
        }

        /// <inheritdoc />
        public override void Dispose()
        {
            TypeUtils.CustomTypes.Remove(this);

            base.Dispose();
        }

        /// <inheritdoc />
        public ScriptType GetType(string typeName)
        {
            // For Visual Script types the typeName is an asset id as Guid string
            if (typeName.Length == 32)
            {
                JsonSerializer.ParseID(typeName, out var id);
                if (Editor.Instance.ContentDatabase.FindAsset(id) is VisualScriptItem visualScriptItem)
                {
                    return visualScriptItem.ScriptType;
                }
            }

            return ScriptType.Null;
        }

        /// <inheritdoc />
        public void GetTypes(List<ScriptType> result, Func<ScriptType, bool> checkFunc)
        {
            var visualScripts = VisualScriptItem.VisualScripts;
            for (var i = 0; i < visualScripts.Count; i++)
            {
                var t = visualScripts[i].ScriptType;
                if (checkFunc(t))
                    result.Add(t);
            }
        }

        /// <inheritdoc />
        public void GetDerivedTypes(ScriptType baseType, List<ScriptType> result, Func<ScriptType, bool> checkFunc)
        {
            var visualScripts = VisualScriptItem.VisualScripts;
            for (var i = 0; i < visualScripts.Count; i++)
            {
                var t = visualScripts[i].ScriptType;
                if (baseType.IsAssignableFrom(t) && t != baseType && checkFunc(t))
                    result.Add(t);
            }
        }
    }
}
