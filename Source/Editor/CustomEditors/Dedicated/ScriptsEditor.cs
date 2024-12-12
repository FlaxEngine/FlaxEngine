// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.Actions;
using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Drag;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;
using Object = FlaxEngine.Object;

namespace FlaxEditor.CustomEditors.Dedicated
{
    internal class NewScriptItem : ItemsListContextMenu.Item
    {
        private string _scriptName;

        public string ScriptName
        {
            get => _scriptName;
            set
            {
                _scriptName = value;
                Name = $"Create script '{value}'";
            }
        }

        public NewScriptItem(string scriptName)
        {
            ScriptName = scriptName;
            TooltipText = "Create a new script";
        }
    }

    /// <summary>
    /// Drag and drop scripts area control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    public class DragAreaControl : ContainerControl
    {
        private DragHandlers _dragHandlers;
        private DragScriptItems _dragScripts;
        private DragAssets _dragAssets;
        private Button _addScriptsButton;

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
            var buttonText = "Add script";
            var textSize = Style.Current.FontMedium.MeasureText(buttonText);
            float addScriptButtonWidth = (textSize.X < 60.0f) ? 60.0f : textSize.X + 4;
            var buttonHeight = (textSize.Y < 18) ? 18 : textSize.Y + 4;
            _addScriptsButton = new Button
            {
                TooltipText = "Add new scripts to the actor",
                AnchorPreset = AnchorPresets.MiddleCenter,
                Text = buttonText,
                Parent = this,
                Bounds = new Rectangle((Width - addScriptButtonWidth) / 2, 1, addScriptButtonWidth, buttonHeight),
            };
            _addScriptsButton.ButtonClicked += OnAddScriptButtonClicked;
        }

        private void OnAddScriptButtonClicked(Button button)
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
                var script = scripts[i];
                var item = new TypeSearchPopup.TypeItemView(script);
                if (script.GetAttributes(false).FirstOrDefault(x => x is RequireActorAttribute) is RequireActorAttribute requireActor)
                {
                    var actors = ScriptsEditor.ParentEditor.Values;
                    foreach (var a in actors)
                    {
                        if (a.GetType() != requireActor.RequiredType)
                        {
                            item.Enabled = false;
                            break;
                        }
                    }
                }
                cm.AddItem(item);
            }
            cm.TextChanged += text =>
            {
                if (!IsValidScriptName(text))
                    return;
                if (!cm.ItemsPanel.Children.Any(x => x.Visible && x is not NewScriptItem))
                {
                    // If there are no visible items, that means the search failed so we can find the create script button or create one if it's the first time
                    var newScriptItem = (NewScriptItem)cm.ItemsPanel.Children.FirstOrDefault(x => x is NewScriptItem);
                    if (newScriptItem != null)
                    {
                        newScriptItem.Visible = true;
                        newScriptItem.ScriptName = text;
                    }
                    else
                    {
                        cm.AddItem(new NewScriptItem(text));
                    }
                }
                else
                {
                    // Make sure to hide the create script button if there
                    var newScriptItem = cm.ItemsPanel.Children.FirstOrDefault(x => x is NewScriptItem);
                    if (newScriptItem != null)
                        newScriptItem.Visible = false;
                }
            };
            cm.ItemClicked += item =>
            {
                if (item.Tag is ScriptType script)
                {
                    AddScript(script);
                }
                else if (item is NewScriptItem newScriptItem)
                {
                    CreateScript(newScriptItem);
                }
            };
            cm.SortItems();
            cm.Show(this, button.BottomLeft - new Float2((cm.Width - button.Width) / 2, 0));
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var size = Size;

            // Info
            Render2D.DrawText(style.FontSmall, "Drag scripts here", new Rectangle(2, _addScriptsButton.Height + 4, size.X - 4, size.Y - 4 - 20), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);

            // Check if drag is over
            if (IsDragOver && _dragHandlers != null && _dragHandlers.HasValidDrag)
            {
                var area = new Rectangle(Float2.Zero, size);
                Render2D.FillRectangle(area, style.Selection);
                Render2D.DrawRectangle(area, style.SelectionBorder);
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

        private static bool IsValidScriptName(string text)
        {
            if (string.IsNullOrEmpty(text))
                return false;
            if (text.Contains(' '))
                return false;
            if (char.IsDigit(text[0]))
                return false;
            if (text.Any(c => !char.IsLetterOrDigit(c) && c != '_'))
                return false;
            return Editor.Instance.ContentDatabase.GetProxy("cs").IsFileNameValid(text);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
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
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None || _dragHandlers == null)
                return result;
            return _dragHandlers.Effect;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            _dragHandlers?.OnDragLeave();

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
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

        private void CreateScript(NewScriptItem item)
        {
            ScriptsEditor.NewScriptName = item.ScriptName;
            var paths = Directory.GetFiles(Globals.ProjectSourceFolder, "*.Build.cs");

            string moduleName = null;
            foreach (var p in paths)
            {
                var file = File.ReadAllText(p);
                if (!file.Contains("GameProjectTarget"))
                    continue; // Skip 

                if (file.Contains("Modules.Add(\"Game\")") || file.Contains("Modules.Add(nameof(Game))"))
                {
                    // Assume Game represents the main game module
                    moduleName = "Game";
                    break;
                }
            }

            // Ensure the path slashes are correct for the OS
            var correctedPath = Path.GetFullPath(Globals.ProjectSourceFolder);
            if (string.IsNullOrEmpty(moduleName))
            {
                var error = FileSystem.ShowBrowseFolderDialog(Editor.Instance.Windows.MainWindow, correctedPath, "Select a module folder to put the new script in", out moduleName);
                if (error)
                    return;
            }
            var path = Path.Combine(Globals.ProjectSourceFolder, moduleName, item.ScriptName + ".cs");
            Editor.Instance.ContentDatabase.GetProxy("cs").Create(path, null);
        }

        /// <summary>
        /// Attach a script to the actor.
        /// </summary>
        /// <param name="item">The script.</param>
        public void AddScript(ScriptType item)
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
            var actions = new List<IUndoAction>();

            for (int i = 0; i < items.Count; i++)
            {
                var scriptType = items[i];
                RequireScriptAttribute scriptAttribute = null;
                if (scriptType.HasAttribute(typeof(RequireScriptAttribute), false))
                {
                    foreach (var e in scriptType.GetAttributes(false))
                    {
                        if (e is not RequireScriptAttribute requireScriptAttribute)
                            continue;
                        scriptAttribute = requireScriptAttribute;
                        break;
                    }
                }

                // See if script requires a specific actor type
                RequireActorAttribute actorAttribute = null;
                if (scriptType.HasAttribute(typeof(RequireActorAttribute), false))
                {
                    foreach (var e in scriptType.GetAttributes(false))
                    {
                        if (e is not RequireActorAttribute requireActorAttribute)
                            continue;
                        actorAttribute = requireActorAttribute;
                        break;
                    }
                }

                var actors = ScriptsEditor.ParentEditor.Values;
                for (int j = 0; j < actors.Count; j++)
                {
                    var actor = (Actor)actors[j];

                    // If required actor exists but is not this actor type then skip adding to actor 
                    if (actorAttribute != null)
                    {
                        if (actor.GetType() != actorAttribute.RequiredType && !actor.GetType().IsSubclassOf(actorAttribute.RequiredType))
                        {
                            Editor.LogWarning($"`{Utilities.Utils.GetPropertyNameUI(scriptType.Name)}` not added to `{actor}` due to script requiring an Actor type of `{actorAttribute.RequiredType}`.");
                            continue;
                        }
                    }

                    actions.Add(AddRemoveScript.Add(actor, scriptType));
                    // Check if actor has required scripts and add them if the actor does not.
                    if (scriptAttribute != null)
                    {
                        foreach (var type in scriptAttribute.RequiredTypes)
                        {
                            if (!type.IsSubclassOf(typeof(Script)))
                            {
                                Editor.LogWarning($"`{Utilities.Utils.GetPropertyNameUI(type.Name)}` not added to `{actor}` due to the class not being a subclass of Script.");
                                continue;
                            }
                            if (actor.GetScript(type) != null)
                                continue;
                            actions.Add(AddRemoveScript.Add(actor, new ScriptType(type)));
                        }
                    }
                }
            }

            if (actions.Count == 0)
            {
                Editor.LogWarning("Failed to spawn scripts");
                return;
            }

            var multiAction = new MultiUndoAction(actions);
            multiAction.Do();
            var presenter = ScriptsEditor.Presenter;
            ScriptsEditor.ParentEditor?.RebuildLayout();
            if (presenter != null)
            {
                presenter.Undo.AddAction(multiAction);
                presenter.Control.Focus();
                
                // Scroll to bottom of script control where a new script is added. 
                if (presenter.Panel.Parent is Panel p && Editor.Instance.Options.Options.Interface.ScrollToScriptOnAdd)
                {
                    var loc = ScriptsEditor.Layout.Control.BottomLeft;
                    p.ScrollViewTo(loc);
                }
            }
        }
    }

    /// <summary>
    /// Small image control added per script group that allows to drag and drop a reference to it. Also used to reorder the scripts.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Image" />
    internal class DragImage : Image
    {
        private bool _isMouseDown;
        private Float2 _mouseDownPos;

        /// <summary>
        /// Action called when drag event should start.
        /// </summary>
        public Action<DragImage> Drag;

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            _mouseDownPos = Float2.Minimum;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (_isMouseDown)
            {
                _isMouseDown = false;
                Drag(this);
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_isMouseDown && Float2.Distance(location, _mouseDownPos) > 10.0f)
            {
                _isMouseDown = false;
                Drag(this);
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _isMouseDown = false;
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _isMouseDown = true;
                _mouseDownPos = location;
                return true;
            }

            return base.OnMouseDown(location, button);
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

            var color = Style.Current.BackgroundSelected * (IsDragOver ? 0.9f : 0.1f);
            Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), color);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
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
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
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
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
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

        /// <summary>
        /// Cached the newly created script name - used to add script after compilation.
        /// </summary>
        internal static string NewScriptName;

        private void AddMissingScript(int index, LayoutElementsContainer layout)
        {
            var group = layout.Group("Missing script");

            // Add settings button to the group
            var settingsButton = group.AddSettingsButton();
            settingsButton.Tag = index;
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

            // If the initialization is triggered by an editor recompilation, check if it was due to script generation from DragAreaControl
            if (NewScriptName != null)
            {
                var script = Editor.Instance.CodeEditing.Scripts.Get().FirstOrDefault(x => x.Name == NewScriptName);
                NewScriptName = null;
                if (script != null)
                {
                    dragArea.CustomControl.AddScript(script);
                }
                else
                {
                    Editor.LogWarning("Failed to find newly created script.");
                }
            }

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
                    var t1 = scripts[j] != null ? scripts[j].TypeName : null;
                    var t2 = e[j] != null ? e[j].TypeName : null;
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

                // Check if actor has all the required scripts
                bool hasAllRequirements = true;
                if (scriptType.HasAttribute(typeof(RequireScriptAttribute), false))
                {
                    RequireScriptAttribute scriptAttribute = null;
                    foreach (var e in scriptType.GetAttributes(false))
                    {
                        if (e is not RequireScriptAttribute requireScriptAttribute)
                            continue;
                        scriptAttribute = requireScriptAttribute;
                    }

                    if (scriptAttribute != null)
                    {
                        foreach (var type in scriptAttribute.RequiredTypes)
                        {
                            if (!type.IsSubclassOf(typeof(Script)))
                                continue;
                            var requiredScript = script.Actor.GetScript(type);
                            if (requiredScript == null)
                            {
                                Editor.LogWarning($"`{Utilities.Utils.GetPropertyNameUI(scriptType.Name)}` on `{script.Actor}` is missing a required Script of type `{type}`.");
                                hasAllRequirements = false;
                            }
                        }
                    }
                }
                if (scriptType.HasAttribute(typeof(RequireActorAttribute), false))
                {
                    RequireActorAttribute attribute = null;
                    foreach (var e in scriptType.GetAttributes(false))
                    {
                        if (e is not RequireActorAttribute requireActorAttribute)
                            continue;
                        attribute = requireActorAttribute;
                        break;
                    }

                    if (attribute != null)
                    {
                        var actor = script.Actor;
                        if (actor.GetType() != attribute.RequiredType && !actor.GetType().IsSubclassOf(attribute.RequiredType))
                        {
                            Editor.LogWarning($"`{Utilities.Utils.GetPropertyNameUI(scriptType.Name)}` on `{script.Actor}` is missing a required Actor of type `{attribute.RequiredType}`.");
                            hasAllRequirements = false;
                            // Maybe call to remove script here?
                        }
                    }
                }

                // Create group
                var title = Utilities.Utils.GetPropertyNameUI(scriptType.Name);
                var group = layout.Group(title, editor);
                if (!hasAllRequirements)
                    group.Panel.HeaderTextColor = FlaxEngine.GUI.Style.Current.Statusbar.Failed;
                if ((Presenter.Features & FeatureFlags.CacheExpandedGroups) != 0)
                {
                    if (Editor.Instance.ProjectCache.IsGroupToggled(title))
                        group.Panel.Close();
                    else
                        group.Panel.Open();
                    group.Panel.IsClosedChanged += panel => Editor.Instance.ProjectCache.SetGroupToggle(panel.HeaderText, panel.IsClosed);
                }
                else
                    group.Panel.Open();

                // Customize
                group.Panel.TooltipText = Editor.Instance.CodeDocs.GetTooltip(scriptType);
                if (script.HasPrefabLink)
                    group.Panel.HeaderTextColor = FlaxEngine.GUI.Style.Current.ProgressNormal;

                // Add toggle button to the group
                var headerHeight = group.Panel.HeaderHeight;
                var scriptToggle = new CheckBox
                {
                    TooltipText = "If checked, script will be enabled.",
                    IsScrollable = false,
                    Checked = script.Enabled,
                    Parent = group.Panel,
                    Size = new Float2(headerHeight),
                    Bounds = new Rectangle(headerHeight, 0, headerHeight, headerHeight),
                    BoxSize = headerHeight - 4.0f,
                    Tag = script,
                };
                scriptToggle.StateChanged += OnScriptToggleCheckChanged;
                _scriptToggles[i] = scriptToggle;

                // Add drag button to the group
                var scriptDrag = new DragImage
                {
                    TooltipText = "Script reference",
                    AutoFocus = true,
                    IsScrollable = false,
                    Color = FlaxEngine.GUI.Style.Current.ForegroundGrey,
                    Parent = group.Panel,
                    Bounds = new Rectangle(scriptToggle.Right, 0.5f, headerHeight, headerHeight),
                    Margin = new Margin(1),
                    Brush = new SpriteBrush(Editor.Instance.Icons.DragBar12),
                    Tag = script,
                    Drag = img =>
                    {
                        var s = (Script)img.Tag;
                        OnScriptDragChange(true, s);
                        img.DoDragDrop(DragScripts.GetDragData(s));
                        OnScriptDragChange(false, s);
                    }
                };

                // Add settings button to the group
                var settingsButton = group.AddSettingsButton();
                settingsButton.Tag = script;
                settingsButton.Clicked += OnSettingsButtonClicked;

                group.Panel.HeaderTextMargin = new Margin(scriptDrag.Right - 12, 15, 2, 2);
                group.Object(values, editor);
                // Remove drop down arrows and containment lines if no objects in the group
                if (group.Children.Count == 0)
                {
                    group.Panel.ArrowImageOpened = null;
                    group.Panel.ArrowImageClosed = null;
                    group.Panel.EnableContainmentLines = false;
                }

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
