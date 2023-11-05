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
    /// UI editor window allows to view and edit <see cref="Widget"/> asset.
    /// </summary>
    /// <seealso cref="WidgetItem" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed partial class UIEditorWindow : AssetEditorWindowBase<JsonAsset>
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="editor"></param>
        /// <param name="item"></param>
        public UIEditorWindow(Editor editor, WidgetItem item) : base(editor, item)
        {
        }
    }
}
