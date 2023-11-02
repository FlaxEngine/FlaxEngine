// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Input;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Prefab window allows to view and edit <see cref="GameUI"/> asset.
    /// </summary>
    /// <seealso cref="GameUI" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed partial class UIEditorWindow : AssetEditorWindowBase<GameUI>
    {
        public UIEditorWindow(Editor editor, AssetItem item) : base(editor, item)
        {
        }
    }
}
