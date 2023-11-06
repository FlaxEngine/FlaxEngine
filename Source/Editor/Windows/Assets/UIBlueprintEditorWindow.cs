// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Collections;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEditor.CustomEditors.Dedicated;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.Viewport;
using FlaxEditor.SceneGraph;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// UI editor window allows to view and edit <see cref="UIBlueprintAsset"/> asset.
    /// </summary>
    /// <seealso cref="UIBlueprintAssetItem" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed partial class UIBlueprintEditorWindow : AssetEditorWindowBase<JsonAsset>, IPresenterOwner
    {
        internal class UIBlueprintEditor : Panel
        {
            /// <summary>
            /// if is true enables the events like on mouse move,update...
            /// </summary>
            public bool Simulate = false;
            public Control Hit = null;
            public Control Selected = null;
            public float Zoom = 1;
            UIBlueprintEditorWindow window;
            private bool isLeftMouseDown;
            private bool isMiddleMouseDown;
            private Float2 mouseStartLocation;
            private Float2 viewOffset;
            public UIBlueprintEditor(UIBlueprintEditorWindow window)
            {
                this.window = window;
            }
            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (Simulate)
                {
                    return base.OnMouseDown(location, button);
                }
                mouseStartLocation = location;
                switch (button)
                {
                    case MouseButton.Left:
                        isLeftMouseDown = Hit != null;
                        if(isLeftMouseDown)
                        {
                            window._propertiesEditor.Select(Hit);
                        }
                        break;
                    case MouseButton.Middle:
                        isMiddleMouseDown = true;
                        break;
                    case MouseButton.Right:
                        break;
                }
                return ContainsPoint(ref location);
            }
            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (Simulate)
                {
                    return base.OnMouseUp(location, button);
                }
                switch (button)
                {
                    case MouseButton.Left:
                        isLeftMouseDown = false;
                        break;
                    case MouseButton.Middle:
                        isMiddleMouseDown = false;
                        break;
                    case MouseButton.Right:
                        break;
                }
                return ContainsPoint(ref location);
            }
            
            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                if (Simulate)
                {
                    base.OnMouseMove(location);
                    return;
                }
                if(isMiddleMouseDown)
                {
                    viewOffset += location - mouseStartLocation;
                    window.Blueprint.Root.Location += location - mouseStartLocation;
                    mouseStartLocation = location;
                    return;
                }
                if (isLeftMouseDown && Hit != null)
                {
                    Hit.Location += location - mouseStartLocation;
                    mouseStartLocation = location;
                }
                else
                {
                    var CL = location - window.Blueprint.Root.Location;
                    if (UIRaycaster.CastRay((ContainerControl)window.Blueprint.Root, ref CL, ref Hit))
                    {
                    }
                    else
                    {
                        Hit = null;
                    }
                }
            }
            /// <inheritdoc />
            public override void DrawSelf()
            {
                base.DrawSelf();
            }
            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();
                if (Hit != null)
                {
                    Render2D.DrawRectangle(new Rectangle(Hit.PointToParent(this,Float2.Zero)+ window.Blueprint.Root.Location, Hit.Size), Color.Green, (1 / (float)Zoom) + 2);
                }
                if (window.Blueprint != null)
                {
                    Render2D.DrawRectangle(window.Blueprint.Root.Bounds, Color.Blue, (1 / (float)Zoom) + 1);
                }
            }
            /// <inheritdoc />
            public override bool OnMouseWheel(Float2 location, float delta)
            {
                Zoom = Math.Clamp(Zoom + delta, 1, 10);
                Scale = new(Zoom);
                return base.OnMouseWheel(location, delta);
            }
        }
        internal UIBlueprintAsset Blueprint;
        internal UIBlueprintEditor BlueprintEditor;
        private CustomEditorPresenter _propertiesEditor;
        private readonly ToolStripButton _saveButton;
        private readonly ToolStripButton _undoButton;
        private readonly ToolStripButton _redoButton;
        private readonly Undo _undo;
        private bool _isRegisteredForScriptsReload;
        internal PropertiesWindow Properties;

        public EditorViewport PresenterViewport => ((IPresenterOwner)Properties).PresenterViewport;

        /// <summary>
        /// 
        /// </summary>
        /// <param name="editor"></param>
        /// <param name="item"></param>
        public UIBlueprintEditorWindow(Editor editor, UIBlueprintAssetItem item) : base(editor, item)
        {
            SplitPanel split = new SplitPanel(Orientation.Horizontal)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, ToolStrip.Bottom, 0),
                Parent = this
            };
            // editor window
            BlueprintEditor = new UIBlueprintEditor(this)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, ToolStrip.Bottom, 0),
                Parent = split.Panel1
            };
            // UI element editor window
            _propertiesEditor = new CustomEditorPresenter(_undo, null,this);
            _propertiesEditor.OverrideEditor = new UIControlControlEditor();
            _propertiesEditor.Panel.Parent = split.Panel2;
            _propertiesEditor.Features |= FeatureFlags.CacheExpandedGroups;
            _propertiesEditor.Select(null);
            _propertiesEditor.Modified += MarkAsEdited;
            _propertiesEditor.SelectionChanged += () => { };
            ToolStrip.AddButton(Editor.Icons.AddFile64, () =>
            {
                if (Blueprint.Root is ContainerControl control) {
                    Image image = control.AddChild<Image>();
                    image.Location = control.Location + control.Size * 0.5f;
                    image.Size = new Float2(32, 32);
                    image.Brush = new SolidColorBrush(Color.White);
                    IsEdited = true;
                }
            }
            );
            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save");
            _toolstrip.AddSeparator();
            _undoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip("Undo (Ctrl+Z)");
            _redoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip("Redo (Ctrl+Y)");

            // Setup input actions
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
        }

        private void OnUndoRedo(IUndoAction action)
        {
            MarkAsEdited();
            UpdateToolstrip();
        }

        private void OnScriptsReloadBegin()
        {
            Close();
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;
            if (_asset.WaitForLoaded())
                return;

            if (Blueprint == null)
                throw new ArgumentNullException();
            string str = FlaxEngine.Json.JsonSerializer.Serialize(Blueprint, true);// widget.Save();
            if (Editor.Internal_SaveJsonAsset(_item.Path, str, Blueprint.GetType().FullName))
            {
                Editor.LogError("Cannot save asset.");
                return;
            }
            
            ClearEditedFlag();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;
            _undoButton.Enabled = _undo.CanUndo;
            _redoButton.Enabled = _undo.CanRedo;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _undo.Clear();
            ClearEditedFlag();
            Blueprint = FlaxEngine.Json.JsonSerializer.Deserialize<UIBlueprintAsset>(Asset.Data);
            // Auto-close on scripting reload if json asset is from game scripts (it might be reloaded)
            if ((Blueprint == null || FlaxEngine.Scripting.IsTypeFromGameScripts(Blueprint.GetType())) && !_isRegisteredForScriptsReload)
            {
                _isRegisteredForScriptsReload = true;
                ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
            }

            base.OnAssetLoaded();
            if (Blueprint == null)
            {
                return;
            }
            else
            {
                Blueprint.Root.Parent = BlueprintEditor;
                Blueprint.Root.AnchorPreset = AnchorPresets.MiddleCenter;
                Blueprint.Root.Size = Screen.Size;
            }
        }
        /// <inheritdoc />
        protected override void OnAssetLoadFailed()
        {
            base.OnAssetLoadFailed();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Refresh the properties (will get new data in OnAssetLoaded)
            ClearEditedFlag();

            base.OnItemReimported(item);
        }
        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_isRegisteredForScriptsReload)
            {
                _isRegisteredForScriptsReload = false;
                ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            }
            base.OnDestroy();
        }

        public void Select(List<SceneGraphNode> nodes)
        {
            ((IPresenterOwner)Properties).Select(nodes);
        }
    }
}
