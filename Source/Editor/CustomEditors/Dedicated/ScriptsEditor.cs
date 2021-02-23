// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Actions;
using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Drag;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Drag and drop scripts area control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    public class DragAreaControl : ContainerControl
    {
        private DragHandlers _dragHandlers;
        private DragScriptItems _dragScripts;
        private DragAssets _dragAssets;

        /// <summary>
        /// The parent scripts editor.
        /// </summary>
        public ScriptsEditor ScriptsEditor;

        /// <summary>
        /// Initializes a new instance of the <see cref="DragAreaControl"/> class.
        /// </summary>
        public DragAreaControl()
        : base(0, 0, 120, 40)
        {
            AutoFocus = false;

            // Add script button
            float addScriptButtonWidth = 60.0f;
            var addScriptButton = new Button
            {
                TooltipText = "Add new scripts to the actor",
                AnchorPreset = AnchorPresets.MiddleCenter,
                Text = "Add script",
                Parent = this,
                Bounds = new Rectangle((Width - addScriptButtonWidth) / 2, 1, addScriptButtonWidth, 18),
            };
            addScriptButton.ButtonClicked += AddScriptButtonOnClicked;
        }

        private void AddScriptButtonOnClicked(Button button)
        {
            var scripts = Editor.Instance.CodeEditing.Scripts.Get();
            if (scripts.Count == 0)
            {
                // No scripts
                var cm1 = new ContextMenu();
                cm1.AddButton("No scripts in project");
                cm1.Show(this, button.BottomLeft);
                return;
            }

            // Show context menu with list of scripts to add
            var cm = new ItemsListContextMenu(180);
            for (int i = 0; i < scripts.Count; i++)
            {
                var scriptType = scripts[i];
                var item = new ItemsListContextMenu.Item(scriptType.Name, scriptType)
                {
                    TooltipText = scriptType.TypeName,
                };
                var attributes = scriptType.GetAttributes(false);
                var tooltipAttribute = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
                if (tooltipAttribute != null)
                {
                    item.TooltipText += '\n';
                    item.TooltipText += tooltipAttribute.Text;
                }
                cm.AddItem(item);
            }
            cm.ItemClicked += item => AddScript((ScriptType)item.Tag);
            cm.SortChildren();
            cm.Show(this, button.BottomLeft);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var size = Size;

            // Info
            Render2D.DrawText(style.FontSmall, "Drag scripts here", new Rectangle(2, 22, size.X - 4, size.Y - 4 - 20), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);

            // Check if drag is over
            if (IsDragOver && _dragHandlers != null && _dragHandlers.HasValidDrag)
            {
                var area = new Rectangle(Vector2.Zero, size);
                Render2D.FillRectangle(area, Color.Orange * 0.5f);
                Render2D.DrawRectangle(area, Color.Black);
            }

            base.Draw();
        }

        private bool ValidateScript(ScriptItem scriptItem)
        {
            var scriptName = scriptItem.ScriptName;
            var scriptType = ScriptsBuilder.FindScript(scriptName);
            return scriptType != null;
        }

        private bool ValidateAsset(AssetItem assetItem)
        {
            if (assetItem is VisualScriptItem scriptItem)
                return scriptItem.ScriptType != ScriptType.Null;
            return false;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            if (_dragHandlers == null)
            {
                _dragScripts = new DragScriptItems(ValidateScript);
                _dragAssets = new DragAssets(ValidateAsset);
                _dragHandlers = new DragHandlers
                {
                    _dragScripts,
                    _dragAssets,
                };
            }
            return _dragHandlers.OnDragEnter(data);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            return _dragHandlers.Effect;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            _dragHandlers.OnDragLeave();

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            if (_dragHandlers.HasValidDrag)
            {
                if (_dragScripts.HasValidDrag)
                {
                    result = _dragScripts.Effect;
                    AddScripts(_dragScripts.Objects);
                }
                else if (_dragAssets.HasValidDrag)
                {
                    result = _dragAssets.Effect;
                    AddScripts(_dragAssets.Objects);
                }

                _dragHandlers.OnDragDrop(null);
            }

            return result;
        }

        private void AddScript(ScriptType item)
        {
            var list = new List<ScriptType>(1) { item };
            AddScripts(list);
        }

        private void AddScripts(List<AssetItem> items)
        {
            var list = new List<ScriptType>(items.Count);
            for (int i = 0; i < items.Count; i++)
            {
                var item = (VisualScriptItem)items[i];
                var scriptType = item.ScriptType;
                if (scriptType == ScriptType.Null)
                {
                    Editor.LogWarning("Invalid script type " + item.ShortName);
                }
                else
                {
                    list.Add(scriptType);
                }
            }
            AddScripts(list);
        }

        private void AddScripts(List<ScriptItem> items)
        {
            var list = new List<ScriptType>(items.Count);
            for (int i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var scriptName = item.ScriptName;
                var scriptType = ScriptsBuilder.FindScript(scriptName);
                if (scriptType == null)
                {
                    Editor.LogWarning("Invalid script type " + scriptName);
                }
                else
                {
                    list.Add(new ScriptType(scriptType));
                }
            }
            AddScripts(list);
        }

        private void AddScripts(List<ScriptType> items)
        {
            var actions = new List<IUndoAction>(4);

            for (int i = 0; i < items.Count; i++)
            {
                var scriptType = items[i];
                var actors = ScriptsEditor.ParentEditor.Values;
                for (int j = 0; j < actors.Count; j++)
                {
                    var actor = (Actor)actors[j];
                    actions.Add(AddRemoveScript.Add(actor, scriptType));
                }
            }

            if (actions.Count == 0)
            {
                Editor.LogWarning("Failed to spawn scripts");
                return;
            }

            var multiAction = new MultiUndoAction(actions);
            multiAction.Do();
            ScriptsEditor.Presenter?.Undo.AddAction(multiAction);
        }
    }

    /// <summary>
    /// Small image control added per script group that allows to drag and drop a reference to it. Also used to reorder the scripts.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Image" />
    internal class ScriptDragIcon : Image
    {
        private ScriptsEditor _editor;
        private bool _isMouseDown;
        private Vector2 _mouseDownPos;

        /// <summary>
        /// Gets the target script.
        /// </summary>
        public Script Script => (Script)Tag;

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptDragIcon"/> class.
        /// </summary>
        /// <param name="editor">The script editor.</param>
        /// <param name="script">The target script.</param>
        public ScriptDragIcon(ScriptsEditor editor, Script script)
        {
            Tag = script;
            _editor = editor;
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Vector2 location)
        {
            _mouseDownPos = Vector2.Minimum;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Check if start drag drop
            if (_isMouseDown)
            {
                DoDrag();
                _isMouseDown = false;
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Vector2 location)
        {
            // Check if start drag drop
            if (_isMouseDown && Vector2.Distance(location, _mouseDownPos) > 10.0f)
            {
                DoDrag();
                _isMouseDown = false;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                // Clear flag
                _isMouseDown = false;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                // Set flag
                _isMouseDown = true;
                _mouseDownPos = location;
            }

            return base.OnMouseDown(location, button);
        }

        private void DoDrag()
        {
            var script = Script;
            _editor.OnScriptDragChange(true, script);
            DoDragDrop(DragScripts.GetDragData(script));
            _editor.OnScriptDragChange(false, script);
        }
    }

    internal class ScriptArrangeBar : Control
    {
        private ScriptsEditor _editor;
        private int _index;
        private Script _script;
        private DragDropEffect _dragEffect;

        public ScriptArrangeBar()
        : base(0, 0, 120, 6)
        {
            AutoFocus = false;
            Visible = false;
        }

        public void Init(int index, ScriptsEditor editor)
        {
            _editor = editor;
            _index = index;
            _editor.ScriptDragChange += OnScriptDragChange;
        }

        private void OnScriptDragChange(bool start, Script script)
        {
            _script = start ? script : null;
            Visible = start;
            OnDragLeave();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var color = FlaxEngine.GUI.Style.Current.BackgroundSelected * (IsDragOver ? 0.9f : 0.1f);
            Render2D.FillRectangle(new Rectangle(Vector2.Zero, Size), color);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            _dragEffect = DragDropEffect.None;

            var result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            if (data is DragDataText textData && DragScripts.IsValidData(textData))
                return _dragEffect = DragDropEffect.Move;

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            return _dragEffect;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            _dragEffect = DragDropEffect.None;

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            if (_dragEffect != DragDropEffect.None)
            {
                result = _dragEffect;
                _dragEffect = DragDropEffect.None;

                _editor.ReorderScript(_script, _index);
            }

            return result;
        }
    }

    /// <summary>
    /// Custom editor for actor scripts collection.
    /// </summary>
    /// <seealso cref="CustomEditor" />
    public sealed class ScriptsEditor : SyncPointEditor
    {
        private CheckBox[] _scriptToggles;

        /// <summary>
        /// Delegate for script drag start and event events.
        /// </summary>
        /// <param name="start">Set to true if drag started, otherwise false.</param>
        /// <param name="script">The target script to reorder.</param>
        public delegate void ScriptDragDelegate(bool start, Script script);

        /// <summary>
        /// Occurs when script drag changes (starts or ends).
        /// </summary>
        public event ScriptDragDelegate ScriptDragChange;

        /// <summary>
        /// The scripts collection. Undo operations are recorder for scripts.
        /// </summary>
        private readonly List<Script> _scripts = new List<Script>();

        /// <inheritdoc />
        public override IEnumerable<object> UndoObjects => _scripts;

        private void AddMissingScript(int index, LayoutElementsContainer layout)
        {
            var group = layout.Group("Missing script");

            // Add settings button to the group
            const float settingsButtonSize = 14;
            var settingsButton = new Image
            {
                TooltipText = "Settings",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.TopRight,
                Parent = group.Panel,
                Bounds = new Rectangle(group.Panel.Width - settingsButtonSize, 0, settingsButtonSize, settingsButtonSize),
                IsScrollable = false,
                Color = FlaxEngine.GUI.Style.Current.ForegroundGrey,
                Margin = new Margin(1),
                Brush = new SpriteBrush(FlaxEngine.GUI.Style.Current.Settings),
                Tag = index,
            };
            settingsButton.Clicked += MissingSettingsButtonOnClicked;
        }

        private void MissingSettingsButtonOnClicked(Image image, MouseButton mouseButton)
        {
            if (mouseButton != MouseButton.Left)
                return;

            var index = (int)image.Tag;

            var cm = new ContextMenu
            {
                Tag = index
            };
            cm.AddButton("Remove", OnClickMissingRemove);
            cm.Show(image, image.Size);
        }

        private void OnClickMissingRemove(ContextMenuButton button)
        {
            var index = (int)button.ParentContextMenu.Tag;
            // TODO: support undo
            var actors = ParentEditor.Values;
            for (int i = 0; i < actors.Count; i++)
            {
                var actor = (Actor)actors[i];
                var script = actor.GetScript(index);
                if (script)
                {
                    script.Parent = null;
                    Object.Destroy(script);
                }
                Editor.Instance.Scene.MarkSceneEdited(actor.Scene);
            }
        }

        /// <summary>
        /// Values container for the collection of the scripts. Helps with prefab linkage and reference value usage (uses Prefab Instance ID rather than index in array).
        /// </summary>
        public sealed class ScriptsContainer : ListValueContainer
        {
            private readonly Guid _prefabObjectId;

            /// <summary>
            /// Gets the prefab object identifier used by the container scripts. Empty if there is no valid linkage to the prefab object.
            /// </summary>
            public Guid PrefabObjectId => _prefabObjectId;

            /// <summary>
            /// Initializes a new instance of the <see cref="ScriptsContainer"/> class.
            /// </summary>
            /// <param name="elementType">Type of the collection elements (script type).</param>
            /// <param name="index">The script index in the actor scripts collection.</param>
            /// <param name="values">The collection values (scripts array).</param>
            public ScriptsContainer(ScriptType elementType, int index, ValueContainer values)
            : base(elementType, index)
            {
                Capacity = values.Count;
                for (int i = 0; i < values.Count; i++)
                {
                    var v = (IList)values[i];
                    Add(v[index]);
                }

                if (values.HasReferenceValue && Count > 0 && this[0] is Script script && script.HasPrefabLink)
                {
                    _prefabObjectId = script.PrefabObjectID;
                    RefreshReferenceValue(values.ReferenceValue);
                }
            }

            /// <inheritdoc />
            public override void RefreshReferenceValue(object instanceValue)
            {
                // Clear
                _referenceValue = null;
                _hasReferenceValue = false;

                if (instanceValue is IList v)
                {
                    // Get the reference value if script with the given link id exists in the reference values collection
                    for (int i = 0; i < v.Count; i++)
                    {
                        if (v[i] is Script script && script.PrefabObjectID == _prefabObjectId)
                        {
                            _referenceValue = script;
                            _hasReferenceValue = true;
                            break;
                        }
                    }
                }
            }
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            // Area for drag&drop scripts
            var dragArea = layout.CustomContainer<DragAreaControl>();
            dragArea.CustomControl.ScriptsEditor = this;

            // No support for showing scripts from multiple actors that have different set of scripts
            var scripts = (Script[])Values[0];
            _scripts.Clear();
            _scripts.AddRange(scripts);
            for (int i = 1; i < Values.Count; i++)
            {
                var e = (Script[])Values[i];
                if (scripts.Length != e.Length)
                    return;
                for (int j = 0; j < e.Length; j++)
                {
                    var t1 = scripts[j]?.TypeName;
                    var t2 = e[j]?.TypeName;
                    if (t1 != t2)
                        return;
                }
            }

            // Scripts arrange bar
            var dragBar = layout.Custom<ScriptArrangeBar>();
            dragBar.CustomControl.Init(0, this);

            // Scripts
            var elementType = new ScriptType(typeof(Script));
            _scriptToggles = new CheckBox[scripts.Length];
            for (int i = 0; i < scripts.Length; i++)
            {
                var script = scripts[i];
                if (script == null)
                {
                    AddMissingScript(i, layout);
                    continue;
                }

                var values = new ScriptsContainer(elementType, i, Values);
                var scriptType = TypeUtils.GetObjectType(script);
                var editor = CustomEditorsUtil.CreateEditor(scriptType, false);

                // Create group
                var title = CustomEditorsUtil.GetPropertyNameUI(scriptType.Name);
                var group = layout.Group(title, editor);
                if (Presenter.CacheExpandedGroups)
                {
                    if (Editor.Instance.ProjectCache.IsCollapsedGroup(title))
                        group.Panel.Close(false);
                    else
                        group.Panel.Open(false);
                    group.Panel.IsClosedChanged += panel => Editor.Instance.ProjectCache.SetCollapsedGroup(panel.HeaderText, panel.IsClosed);
                }
                else
                    group.Panel.Open(false);

                // Customize
                var typeAttributes = scriptType.GetAttributes(true);
                var tooltip = (TooltipAttribute)typeAttributes.FirstOrDefault(x => x is TooltipAttribute);
                if (tooltip != null)
                    group.Panel.TooltipText = tooltip.Text;
                if (script.HasPrefabLink)
                    group.Panel.HeaderTextColor = FlaxEngine.GUI.Style.Current.ProgressNormal;

                // Add toggle button to the group
                var scriptToggle = new CheckBox
                {
                    TooltipText = "If checked, script will be enabled.",
                    IsScrollable = false,
                    Checked = script.Enabled,
                    Parent = group.Panel,
                    Size = new Vector2(14, 14),
                    Bounds = new Rectangle(2, 0, 14, 14),
                    BoxSize = 12.0f,
                    Tag = script,
                };
                scriptToggle.StateChanged += OnScriptToggleCheckChanged;
                _scriptToggles[i] = scriptToggle;

                // Add drag button to the group
                const float dragIconSize = 14;
                var scriptDrag = new ScriptDragIcon(this, script)
                {
                    TooltipText = "Script reference",
                    AutoFocus = true,
                    IsScrollable = false,
                    Color = FlaxEngine.GUI.Style.Current.ForegroundGrey,
                    Parent = group.Panel,
                    Bounds = new Rectangle(scriptToggle.Right, 0.5f, dragIconSize, dragIconSize),
                    Margin = new Margin(1),
                    Brush = new SpriteBrush(Editor.Instance.Icons.DragBar12),
                    Tag = script,
                };

                // Add settings button to the group
                const float settingsButtonSize = 14;
                var settingsButton = new Image
                {
                    TooltipText = "Settings",
                    AutoFocus = true,
                    AnchorPreset = AnchorPresets.TopRight,
                    Parent = group.Panel,
                    Bounds = new Rectangle(group.Panel.Width - settingsButtonSize, 0, settingsButtonSize, settingsButtonSize),
                    IsScrollable = false,
                    Color = FlaxEngine.GUI.Style.Current.ForegroundGrey,
                    Margin = new Margin(1),
                    Brush = new SpriteBrush(FlaxEngine.GUI.Style.Current.Settings),
                    Tag = script,
                };
                settingsButton.Clicked += OnSettingsButtonClicked;

                group.Panel.HeaderTextMargin = new Margin(scriptDrag.Right, 15, 2, 2);
                group.Object(values, editor);

                // Scripts arrange bar
                dragBar = layout.Custom<ScriptArrangeBar>();
                dragBar.CustomControl.Init(i + 1, this);
            }

            base.Initialize(layout);
        }

        /// <summary>
        /// Called when script drag changes.
        /// </summary>
        /// <param name="start">if set to <c>true</c> drag just started, otherwise ended.</param>
        /// <param name="script">The target script.</param>
        public void OnScriptDragChange(bool start, Script script)
        {
            ScriptDragChange.Invoke(start, script);
        }

        /// <summary>
        /// Changes the script order (with undo).
        /// </summary>
        /// <param name="script">The script to reorder.</param>
        /// <param name="targetIndex">The target index to move script.</param>
        public void ReorderScript(Script script, int targetIndex)
        {
            // Skip if no change
            if (script.OrderInParent == targetIndex)
                return;

            var action = ChangeScriptAction.ChangeOrder(script, targetIndex);
            action.Do();
            Presenter?.Undo.AddAction(action);
        }

        private void OnScriptToggleCheckChanged(CheckBox box)
        {
            var script = (Script)box.Tag;
            if (script.Enabled == box.Checked)
                return;

            var action = ChangeScriptAction.ChangeEnabled(script, box.Checked);
            action.Do();
            Presenter?.Undo.AddAction(action);
        }

        private void OnSettingsButtonClicked(Image image, MouseButton mouseButton)
        {
            if (mouseButton != MouseButton.Left)
                return;

            var script = (Script)image.Tag;
            var scriptType = TypeUtils.GetType(script.TypeName);
            var item = scriptType.ContentItem;

            var cm = new ContextMenu
            {
                Tag = script
            };
            cm.AddButton("Remove", OnClickRemove).Icon = Editor.Instance.Icons.Cross12;
            cm.AddButton("Move up", OnClickMoveUp).Enabled = script.OrderInParent > 0;
            cm.AddButton("Move down", OnClickMoveDown).Enabled = script.OrderInParent < script.Actor.Scripts.Length - 1;
            // TODO: copy script
            // TODO: paste script values
            // TODO: paste script as new
            // TODO: copy script reference
            cm.AddSeparator();
            cm.AddButton("Copy type name", OnClickCopyTypeName);
            cm.AddButton("Edit script", OnClickEditScript).Enabled = item != null;
            var showButton = cm.AddButton("Show in content window", OnClickShowScript);
            showButton.Enabled = item != null;
            showButton.Icon = Editor.Instance.Icons.Search12;
            cm.Show(image, image.Size);
        }

        private void OnClickRemove(ContextMenuButton button)
        {
            var script = (Script)button.ParentContextMenu.Tag;
            var action = AddRemoveScript.Remove(script);
            action.Do();
            Presenter.Undo?.AddAction(action);
        }

        private void OnClickMoveUp(ContextMenuButton button)
        {
            var script = (Script)button.ParentContextMenu.Tag;
            var action = ChangeScriptAction.ChangeOrder(script, script.OrderInParent - 1);
            action.Do();
            Presenter.Undo?.AddAction(action);
        }

        private void OnClickMoveDown(ContextMenuButton button)
        {
            var script = (Script)button.ParentContextMenu.Tag;
            var action = ChangeScriptAction.ChangeOrder(script, script.OrderInParent + 1);
            action.Do();
            Presenter.Undo?.AddAction(action);
        }

        private void OnClickCopyTypeName(ContextMenuButton button)
        {
            var script = (Script)button.ParentContextMenu.Tag;
            Clipboard.Text = script.TypeName;
        }

        private void OnClickEditScript(ContextMenuButton button)
        {
            var script = (Script)button.ParentContextMenu.Tag;
            var scriptType = TypeUtils.GetType(script.TypeName);
            var item = scriptType.ContentItem;
            if (item != null)
                Editor.Instance.ContentEditing.Open(item);
        }

        private void OnClickShowScript(ContextMenuButton button)
        {
            var script = (Script)button.ParentContextMenu.Tag;
            var scriptType = TypeUtils.GetType(script.TypeName);
            var item = scriptType.ContentItem;
            if (item != null)
                Editor.Instance.Windows.ContentWin.Select(item);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            if (Values.Count == 1)
            {
                var scripts = ((Actor)ParentEditor.Values[0]).Scripts;
                if (!Utils.ArraysEqual(scripts, _scripts))
                {
                    ParentEditor.RebuildLayout();
                    return;
                }

                for (int i = 0; i < _scriptToggles.Length; i++)
                {
                    if (_scriptToggles[i] != null)
                        _scriptToggles[i].Checked = scripts[i].Enabled;
                }
            }

            base.Refresh();
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _scriptToggles = null;

            base.Deinitialize();
        }
    }
}
