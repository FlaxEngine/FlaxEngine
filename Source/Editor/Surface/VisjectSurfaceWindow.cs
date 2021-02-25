// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.History;
using FlaxEditor.Scripting;
using FlaxEditor.Viewport.Previews;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// The base interface for editor windows that use <see cref="FlaxEditor.Surface.VisjectSurface"/> for content editing. 
    /// </summary>
    /// <seealso cref="FlaxEditor.Surface.IVisjectSurfaceOwner" />
    interface IVisjectSurfaceWindow : IVisjectSurfaceOwner
    {
        /// <summary>
        /// Gets the asset edited by the window.
        /// </summary>
        Asset VisjectAsset { get; }

        /// <summary>
        /// Gets the Visject surface editor.
        /// </summary>
        VisjectSurface VisjectSurface { get; }

        /// <summary>
        /// The new parameter types enum type to use. Null to disable adding new parameters.
        /// </summary>
        IEnumerable<ScriptType> NewParameterTypes { get; }

        /// <summary>
        /// Called when parameter rename undo action is performed.
        /// </summary>
        void OnParamRenameUndo();

        /// <summary>
        /// Called when parameter edit attributes undo action is performed.
        /// </summary>
        void OnParamEditAttributesUndo();

        /// <summary>
        /// Called when parameter add undo action is performed.
        /// </summary>
        void OnParamAddUndo();

        /// <summary>
        /// Called when parameter remove undo action is performed.
        /// </summary>
        void OnParamRemoveUndo();

        /// <summary>
        /// Gets the asset parameter.
        /// </summary>
        /// <param name="index">The zero-based parameter index.</param>
        /// <returns>The value.</returns>
        object GetParameter(int index);

        /// <summary>
        /// Sets the asset parameter.
        /// </summary>
        /// <param name="index">The zero-based parameter index.</param>
        /// <param name="value">The value to set.</param>
        void SetParameter(int index, object value);
    }

    /// <summary>
    /// The surface parameter rename action for undo.
    /// </summary>
    /// <seealso cref="IVisjectSurfaceWindow" />
    sealed class RenameParamAction : IUndoAction
    {
        /// <summary>
        /// The window reference.
        /// </summary>
        public IVisjectSurfaceWindow Window;

        /// <summary>
        /// The index of the parameter.
        /// </summary>
        public int Index;

        /// <summary>
        /// The name before.
        /// </summary>
        public string Before;

        /// <summary>
        /// The name after.
        /// </summary>
        public string After;

        /// <inheritdoc />
        public string ActionString => "Rename parameter";

        /// <inheritdoc />
        public void Do()
        {
            Set(After);
        }

        /// <inheritdoc />
        public void Undo()
        {
            Set(Before);
        }

        private void Set(string value)
        {
            var param = Window.VisjectSurface.Parameters[Index];
            param.Name = value;
            Window.VisjectSurface.OnParamRenamed(param);
            Window.OnParamRenameUndo();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            Window = null;
            Before = null;
            After = null;
        }
    }

    /// <summary>
    /// The attributes edit action for undo.
    /// </summary>
    /// <seealso cref="IVisjectSurfaceWindow" />
    abstract class EditAttributesAction : IUndoAction
    {
        /// <summary>
        /// The window reference.
        /// </summary>
        public IVisjectSurfaceWindow Window;

        /// <summary>
        /// The attributes before.
        /// </summary>
        public Attribute[] Before;

        /// <summary>
        /// The attributes after.
        /// </summary>
        public Attribute[] After;

        /// <inheritdoc />
        public string ActionString => "Edit attributes";

        /// <inheritdoc />
        public void Do()
        {
            Set(After);
        }

        /// <inheritdoc />
        public void Undo()
        {
            Set(Before);
        }

        /// <summary>
        /// Sets the specified attributes.
        /// </summary>
        /// <param name="value">The value.</param>
        protected abstract void Set(Attribute[] value);

        /// <inheritdoc />
        public void Dispose()
        {
            Window = null;
            Before = null;
            After = null;
        }
    }

    /// <summary>
    /// The surface attributes edit action for undo.
    /// </summary>
    /// <seealso cref="IVisjectSurfaceWindow" />
    sealed class EditSurfaceAttributesAction : EditAttributesAction
    {
        /// <inheritdoc />
        protected override void Set(Attribute[] value)
        {
            Window.VisjectSurface.Context.Meta.SetAttributes(value);
            Window.VisjectSurface.MarkAsEdited();
        }
    }

    /// <summary>
    /// The surface node attributes edit action for undo.
    /// </summary>
    /// <seealso cref="IVisjectSurfaceWindow" />
    sealed class EditNodeAttributesAction : EditAttributesAction
    {
        /// <summary>
        /// The id of the node.
        /// </summary>
        public uint ID;

        /// <inheritdoc />
        protected override void Set(Attribute[] value)
        {
            var node = Window.VisjectSurface.FindNode(ID);
            node.Meta.SetAttributes(value);
            Window.VisjectSurface.MarkAsEdited();
        }
    }

    /// <summary>
    /// The surface parameter attributes edit action for undo.
    /// </summary>
    /// <seealso cref="IVisjectSurfaceWindow" />
    sealed class EditParamAttributesAction : EditAttributesAction
    {
        /// <summary>
        /// The index of the parameter.
        /// </summary>
        public int Index;

        /// <inheritdoc />
        protected override void Set(Attribute[] value)
        {
            var param = Window.VisjectSurface.Parameters[Index];
            param.Meta.SetAttributes(value);
            Window.VisjectSurface.MarkAsEdited();
            Window.OnParamEditAttributesUndo();
        }
    }

    /// <summary>
    /// The undo action for adding or removing surface parameter.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Assets.ClonedAssetEditorWindowBase{TAsset}" />
    /// <seealso cref="FlaxEditor.Surface.IVisjectSurfaceOwner" />
    sealed class AddRemoveParamAction : IUndoAction
    {
        /// <summary>
        /// The window reference.
        /// </summary>
        public IVisjectSurfaceWindow Window;

        /// <summary>
        /// True if adding, false if removing parameter.
        /// </summary>
        public bool IsAdd;

        /// <summary>
        /// The index of the parameter.
        /// </summary>
        public int Index;

        /// <summary>
        /// The name of the parameter.
        /// </summary>
        public string Name;

        /// <summary>
        /// The type of the parameter.
        /// </summary>
        public ScriptType Type;

        /// <inheritdoc />
        public string ActionString => IsAdd ? "Add parameter" : "Remove parameter";

        /// <inheritdoc />
        public void Do()
        {
            if (IsAdd)
                Add();
            else
                Remove();
        }

        /// <inheritdoc />
        public void Undo()
        {
            if (IsAdd)
                Remove();
            else
                Add();
        }

        private void Add()
        {
            var type = Type;
            if (IsAdd && type.Type == typeof(NormalMap))
                type = new ScriptType(typeof(Texture));
            var param = SurfaceParameter.Create(type, Name);
            if (IsAdd && Type.Type == typeof(NormalMap))
                param.Value = FlaxEngine.Content.LoadAsyncInternal<Texture>("Engine/Textures/NormalTexture");
            Window.VisjectSurface.Parameters.Insert(Index, param);
            Window.VisjectSurface.OnParamCreated(param);
            Window.OnParamAddUndo();
        }

        private void Remove()
        {
            var param = Window.VisjectSurface.Parameters[Index];
            if (!IsAdd)
            {
                Name = param.Name;
                Type = param.Type;
            }
            Window.VisjectSurface.Parameters.RemoveAt(Index);
            Window.VisjectSurface.OnParamDeleted(param);
            Window.OnParamRemoveUndo();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            Window = null;
        }
    }

    /// <summary>
    /// Custom editor for editing Visject Surface parameters collection.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.CustomEditor" />
    public class ParametersEditor : CustomEditor
    {
        private static readonly Attribute[] DefaultAttributes =
        {
            //new LimitAttribute(float.MinValue, float.MaxValue, 0.1f),
        };

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.InlineIntoParent;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var window = Values[0] as IVisjectSurfaceWindow;
            var asset = window?.VisjectAsset;
            if (asset == null)
            {
                layout.Label("No parameters");
                return;
            }
            if (asset.LastLoadFailed)
            {
                layout.Label("Failed to load asset");
                return;
            }
            if (!asset.IsLoaded)
            {
                layout.Label("Loading...");
                return;
            }
            var parameters = window.VisjectSurface.Parameters;
            GroupElement lastGroup = null;

            for (int i = 0; i < parameters.Count; i++)
            {
                var p = parameters[i];
                if (!p.IsPublic)
                    continue;

                var pIndex = i;
                var pValue = p.Value;
                var attributes = p.Meta.GetAttributes();
                if (attributes == null || attributes.Length == 0)
                    attributes = DefaultAttributes;
                var itemLayout = layout;
                var name = p.Name;

                // Editor Display
                var editorDisplay = (EditorDisplayAttribute)attributes.FirstOrDefault(x => x is EditorDisplayAttribute);
                if (editorDisplay?.Group != null)
                {
                    if (lastGroup == null || lastGroup.Panel.HeaderText != editorDisplay.Group)
                    {
                        lastGroup = layout.Group(editorDisplay.Group);
                        lastGroup.Panel.Open(false);
                    }
                    itemLayout = lastGroup;
                }
                else
                {
                    lastGroup = null;
                    itemLayout = layout;
                }
                if (editorDisplay?.Name != null)
                    name = editorDisplay.Name;

                // Space
                var space = (SpaceAttribute)attributes.FirstOrDefault(x => x is SpaceAttribute);
                if (space != null)
                    itemLayout.Space(space.Height);

                // Header
                var header = (HeaderAttribute)attributes.FirstOrDefault(x => x is HeaderAttribute);
                if (header != null)
                    itemLayout.Header(header.Text);

                var propertyValue = new CustomValueContainer
                (
                 p.Type,
                 pValue,
                 (instance, index) => ((IVisjectSurfaceWindow)instance).GetParameter(pIndex),
                 (instance, index, value) => ((IVisjectSurfaceWindow)instance).SetParameter(pIndex, value),
                 attributes
                );

                var propertyLabel = new DraggablePropertyNameLabel(name)
                {
                    Tag = pIndex,
                    Drag = OnDragParameter
                };
                var tooltip = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
                propertyLabel.MouseLeftDoubleClick += (label, location) => StartParameterRenaming(pIndex, label);
                propertyLabel.SetupContextMenu += OnPropertyLabelSetupContextMenu;
                var property = itemLayout.AddPropertyItem(propertyLabel, tooltip?.Text);
                property.Object(propertyValue);
            }

            // Parameters creating
            var newParameterTypes = window.NewParameterTypes;
            if (newParameterTypes != null)
            {
                layout.Space(parameters.Count > 0 ? 10 : 4);
                var newParam = layout.Button("Add parameter...");
                newParam.Button.ButtonClicked += OnAddParameterButtonClicked;
                layout.Space(10);
            }
        }

        private void OnAddParameterButtonClicked(Button button)
        {
            var window = (IVisjectSurfaceWindow)Values[0];
            var newParameterTypes = window.NewParameterTypes;

            // Show context menu with list of parameter types to add
            var cm = new ItemsListContextMenu(180);
            foreach (var newParameterType in newParameterTypes)
            {
                var name = newParameterType.Type != null ? window.VisjectSurface.GetTypeName(newParameterType) : newParameterType.Name;
                var item = new ItemsListContextMenu.Item(name, newParameterType)
                {
                    TooltipText = newParameterType.TypeName,
                };
                var attributes = newParameterType.GetAttributes(false);
                var tooltipAttribute = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
                if (tooltipAttribute != null)
                {
                    item.TooltipText += '\n';
                    item.TooltipText += tooltipAttribute.Text;
                }
                cm.AddItem(item);
            }
            cm.ItemClicked += OnAddParameterItemClicked;
            cm.SortChildren();
            cm.Show(button.Parent, button.BottomLeft);
        }

        private void OnAddParameterItemClicked(ItemsListContextMenu.Item item)
        {
            var type = (ScriptType)item.Tag;
            var window = (IVisjectSurfaceWindow)Values[0];
            var asset = window?.VisjectAsset;
            if (asset == null || !asset.IsLoaded)
                return;
            var action = new AddRemoveParamAction
            {
                Window = window,
                IsAdd = true,
                Name = StringUtils.IncrementNameNumber("New parameter", x => OnParameterRenameValidate(null, x)),
                Type = type,
                Index = window.VisjectSurface.Parameters.Count,
            };
            window.VisjectSurface.Undo.AddAction(action);
            action.Do();
        }

        private DragData OnDragParameter(DraggablePropertyNameLabel label)
        {
            var window = (IVisjectSurfaceWindow)Values[0];
            var parameter = window.VisjectSurface.Parameters[(int)label.Tag];
            return DragNames.GetDragData(SurfaceParameter.DragPrefix, parameter.Name);
        }

        private void OnPropertyLabelSetupContextMenu(PropertyNameLabel label, FlaxEditor.GUI.ContextMenu.ContextMenu menu, CustomEditor linkedEditor)
        {
            var index = (int)label.Tag;
            menu.AddSeparator();
            menu.AddButton("Rename", () => StartParameterRenaming(index, label));
            menu.AddButton("Edit attributes...", () => EditAttributesParameter(index, label));
            menu.AddButton("Delete", () => DeleteParameter(index));
        }

        private void StartParameterRenaming(int index, Control label)
        {
            var window = (IVisjectSurfaceWindow)Values[0];
            var parameter = window.VisjectSurface.Parameters[(int)label.Tag];
            var dialog = RenamePopup.Show(label, new Rectangle(0, 0, label.Width - 2, label.Height), parameter.Name, false);
            dialog.Tag = index;
            dialog.Validate += OnParameterRenameValidate;
            dialog.Renamed += OnParameterRenamed;
        }

        private bool OnParameterRenameValidate(RenamePopup popup, string value)
        {
            var window = (IVisjectSurfaceWindow)Values[0];
            return !string.IsNullOrWhiteSpace(value) && window.VisjectSurface.Parameters.All(x => x.Name != value);
        }

        private void OnParameterRenamed(RenamePopup renamePopup)
        {
            var window = (IVisjectSurfaceWindow)Values[0];
            var index = (int)renamePopup.Tag;
            var action = new RenameParamAction
            {
                Window = window,
                Index = index,
                Before = window.VisjectSurface.Parameters[index].Name,
                After = renamePopup.Text,
            };
            window.VisjectSurface.Undo.AddAction(action);
            action.Do();
        }

        private void EditAttributesParameter(int index, Control label)
        {
            var window = (IVisjectSurfaceWindow)Values[0];
            var attributes = window.VisjectSurface.Parameters[index].Meta.GetAttributes();
            var editor = new AttributesEditor(attributes, NodeFactory.ParameterAttributeTypes);
            editor.Edited += newValue =>
            {
                var action = new EditParamAttributesAction
                {
                    Window = window,
                    Index = index,
                    Before = window.VisjectSurface.Parameters[index].Meta.GetAttributes(),
                    After = newValue,
                };
                window.VisjectSurface.Undo.AddAction(action);
                action.Do();
            };
            editor.Show(label, label.Center);
        }

        private void DeleteParameter(int index)
        {
            var window = (IVisjectSurfaceWindow)Values[0];
            var action = new AddRemoveParamAction
            {
                Window = window,
                IsAdd = false,
                Index = index,
            };
            window.VisjectSurface.Undo.AddAction(action);
            action.Do();
        }
    }

    /// <summary>
    /// Dummy class to inject Normal Map parameter type for the material parameter adding picker.
    /// </summary>
    [Tooltip("Texture asset contains a normal map that is stored on a GPU and is used during rendering graphics to implement normal mapping (aka bump mapping).")]
    internal sealed class NormalMap
    {
    }

    /// <summary>
    /// The base class for editor windows that use <see cref="FlaxEditor.Surface.VisjectSurface"/> for content editing. 
    /// Note: it uses ClonedAssetEditorWindowBase which is creating cloned asset to edit/preview.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    /// <seealso cref="FlaxEditor.Surface.IVisjectSurfaceOwner" />
    /// <seealso cref="IVisjectSurfaceWindow" />
    public abstract class VisjectSurfaceWindow<TAsset, TSurface, TPreview> : ClonedAssetEditorWindowBase<TAsset>, IVisjectSurfaceWindow
    where TAsset : Asset
    where TSurface : VisjectSurface
    where TPreview : AssetPreview
    {
        /// <summary>
        /// The tab.
        /// </summary>
        /// <seealso cref="FlaxEditor.Windows.Assets.ClonedAssetEditorWindowBase{TAsset}" />
        /// <seealso cref="FlaxEditor.Surface.IVisjectSurfaceWindow" />
        protected class Tab : FlaxEditor.GUI.Tabs.Tab
        {
            /// <summary>
            /// The presenter.
            /// </summary>
            public CustomEditorPresenter Presenter;

            /// <summary>
            /// Initializes a new instance of the <see cref="Tab"/> class.
            /// </summary>
            /// <param name="text">The tab title text.</param>
            /// <param name="undo">The undo to use for the editing.</param>
            public Tab(string text, FlaxEditor.Undo undo = null)
            : base(text)
            {
                var scrollPanel = new Panel(ScrollBars.Vertical)
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                    Parent = this
                };

                Presenter = new CustomEditorPresenter(undo);
                Presenter.Panel.Parent = scrollPanel;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                Presenter.Deselect();
                Presenter = null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// The primary split panel.
        /// </summary>
        protected readonly SplitPanel _split1;

        /// <summary>
        /// The secondary split panel.
        /// </summary>
        protected readonly SplitPanel _split2;

        /// <summary>
        /// The asset preview.
        /// </summary>
        protected TPreview _preview;

        /// <summary>
        /// The surface.
        /// </summary>
        protected TSurface _surface;

        /// <summary>
        /// The tabs control. Valid only if window is using tabs instead of just properties.
        /// </summary>
        protected Tabs _tabs;

        private readonly ToolStripButton _saveButton;
        private readonly ToolStripButton _undoButton;
        private readonly ToolStripButton _redoButton;
        private bool _showWholeGraphOnLoad = true;

        /// <summary>
        /// The properties editor.
        /// </summary>
        protected CustomEditorPresenter _propertiesEditor;

        /// <summary>
        /// True if temporary asset is dirty, otherwise false.
        /// </summary>
        protected bool _tmpAssetIsDirty;

        /// <summary>
        /// True if window is waiting for asset load to load surface.
        /// </summary>
        protected bool _isWaitingForSurfaceLoad;

        /// <summary>
        /// True if window is waiting for asset load to refresh properties editor.
        /// </summary>
        protected bool _refreshPropertiesOnLoad;

        /// <summary>
        /// True if parameter value has been changed (special path for handling modifying surface parameters in properties editor).
        /// </summary>
        protected bool _paramValueChange;

        /// <summary>
        /// The undo.
        /// </summary>
        protected FlaxEditor.Undo _undo;

        /// <summary>
        /// Gets the Visject Surface.
        /// </summary>
        public TSurface Surface => _surface;

        /// <summary>
        /// Gets the asset preview.
        /// </summary>
        public TPreview Preview => _preview;

        /// <summary>
        /// Gets the undo history context for this window.
        /// </summary>
        public FlaxEditor.Undo Undo => _undo;

        /// <summary>
        /// Initializes a new instance of the <see cref="VisjectSurfaceWindow{TAsset, TSurface, TPreview}"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        /// <param name="item">The item.</param>
        /// <param name="useTabs">if set to <c>true</c> [use tabs].</param>
        protected VisjectSurfaceWindow(Editor editor, AssetItem item, bool useTabs = false)
        : base(editor, item)
        {
            // Undo
            _undo = new FlaxEditor.Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            // Split Panel 1
            _split1 = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.7f,
                Parent = this
            };

            // Split Panel 2
            _split2 = new SplitPanel(Orientation.Vertical, ScrollBars.None, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                SplitterValue = 0.4f,
                Parent = _split1.Panel2
            };

            // Properties editor
            if (useTabs)
            {
                _tabs = new Tabs
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                    TabsSize = new Vector2(60, 20),
                    TabsTextHorizontalAlignment = TextAlignment.Center,
                    UseScroll = true,
                    Parent = _split2.Panel2
                };
                var propertiesTab = new Tab("Properties", _undo);
                _propertiesEditor = propertiesTab.Presenter;
                _tabs.AddTab(propertiesTab);
            }
            else
            {
                _propertiesEditor = new CustomEditorPresenter(_undo);
                _propertiesEditor.Panel.Parent = _split2.Panel2;
            }
            _propertiesEditor.Modified += OnPropertyEdited;

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Save32, Save).LinkTooltip("Save");
            _toolstrip.AddSeparator();
            _undoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Undo32, _undo.PerformUndo).LinkTooltip("Undo (Ctrl+Z)");
            _redoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Redo32, _undo.PerformRedo).LinkTooltip("Redo (Ctrl+Y)");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.PageScale32, ShowWholeGraph).LinkTooltip("Show whole graph");

            // Setup input actions
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
        }

        private void OnUndoRedo(IUndoAction action)
        {
            // Hack for emitter properties proxy object
            if (action is MultiUndoAction multiUndo &&
                multiUndo.Actions.Length == 1 &&
                multiUndo.Actions[0] is UndoActionObject undoActionObject &&
                undoActionObject.Target == _propertiesEditor.Selection[0])
            {
                OnPropertyEdited();
                UpdateToolstrip();
                return;
            }

            _paramValueChange = false;
            MarkAsEdited();
            UpdateToolstrip();
            _propertiesEditor.BuildLayoutOnUpdate();
        }

        /// <summary>
        /// Called when the asset properties proxy object gets edited.
        /// </summary>
        protected virtual void OnPropertyEdited()
        {
            _surface.MarkAsEdited(!_paramValueChange);
            _paramValueChange = false;
        }

        /// <summary>
        /// Shows the whole surface graph.
        /// </summary>
        public void ShowWholeGraph()
        {
            _surface.ShowWholeGraph();
        }

        /// <summary>
        /// Refreshes temporary asset to see changes live when editing the surface.
        /// </summary>
        /// <returns>True if cannot refresh it, otherwise false.</returns>
        public bool RefreshTempAsset()
        {
            // Early check
            if (_asset == null || _isWaitingForSurfaceLoad)
                return true;

            // Check if surface has been edited
            if (_surface.IsEdited)
            {
                return SaveSurface();
            }

            return false;
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            if (RefreshTempAsset())
            {
                return;
            }

            if (SaveToOriginal())
            {
                return;
            }

            ClearEditedFlag();
            OnSurfaceEditedChanged();
            _item.RefreshThumbnail();
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
        protected override void UnlinkItem()
        {
            _isWaitingForSurfaceLoad = false;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _isWaitingForSurfaceLoad = true;
            _refreshPropertiesOnLoad = false;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        public abstract string SurfaceName { get; }

        /// <inheritdoc />
        public abstract byte[] SurfaceData { get; set; }

        /// <inheritdoc />
        public void OnContextCreated(VisjectSurfaceContext context)
        {
        }

        /// <inheritdoc />
        public void OnSurfaceEditedChanged()
        {
            if (_surface.IsEdited)
                MarkAsEdited();
        }

        /// <inheritdoc />
        public void OnSurfaceGraphEdited()
        {
            // Mark as dirty
            _tmpAssetIsDirty = true;
        }

        /// <inheritdoc />
        public void OnSurfaceClose()
        {
            Close();
        }

        /// <summary>
        /// Loads the surface from the asset. Called during <see cref="Update"/> when asset is loaded and surface is missing.
        /// </summary>
        /// <returns>True if failed, otherwise false.</returns>
        protected abstract bool LoadSurface();

        /// <summary>
        /// Saves the surface to the asset. Called during <see cref="Update"/> when asset is loaded and surface is missing.
        /// </summary>
        /// <returns>True if failed, otherwise false.</returns>
        protected abstract bool SaveSurface();

        /// <summary>
        /// Gets a value indicating whether this window can edit asset surface on asset load error (eg. to fix asset loading issue due to graph problem).
        /// </summary>
        protected virtual bool CanEditSurfaceOnAssetLoadError => false;

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            if (_tmpAssetIsDirty)
            {
                _tmpAssetIsDirty = false;

                RefreshTempAsset();
            }

            if (_isWaitingForSurfaceLoad && (_asset.IsLoaded || (CanEditSurfaceOnAssetLoadError && _asset.LastLoadFailed)))
            {
                _isWaitingForSurfaceLoad = false;
                if (!_asset.IsLoaded)
                {
                    Editor.LogWarning("Loading surface for asset that is not loaded: " + OriginalAsset);
                }

                if (LoadSurface())
                {
                    Close();
                    return;
                }

                // Setup
                _undo.Clear();
                _surface.Enabled = true;
                _propertiesEditor.BuildLayout();
                ClearEditedFlag();
                if (_showWholeGraphOnLoad)
                {
                    _showWholeGraphOnLoad = false;
                    _surface.ShowWholeGraph();
                }
            }
            else if (_refreshPropertiesOnLoad && _asset.IsLoaded)
            {
                _refreshPropertiesOnLoad = false;

                _propertiesEditor.BuildLayout();
            }
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            writer.WriteAttributeString("Split1", _split1.SplitterValue.ToString());
            writer.WriteAttributeString("Split2", _split2.SplitterValue.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {

            if (float.TryParse(node.GetAttribute("Split1"), out float value1))
                _split1.SplitterValue = value1;

            if (float.TryParse(node.GetAttribute("Split2"), out value1))
                _split2.SplitterValue = value1;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split1.SplitterValue = 0.7f;
            _split2.SplitterValue = 0.4f;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _undo.Enabled = false;
            _propertiesEditor.Deselect();
            _undo.Clear();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public abstract IEnumerable<ScriptType> NewParameterTypes { get; }

        /// <inheritdoc />
        public virtual void OnParamRenameUndo()
        {
        }

        /// <inheritdoc />
        public virtual void OnParamEditAttributesUndo()
        {
            _propertiesEditor.BuildLayout();
        }

        /// <inheritdoc />
        public virtual void OnParamAddUndo()
        {
            _refreshPropertiesOnLoad = true;
        }

        /// <inheritdoc />
        public virtual void OnParamRemoveUndo()
        {
            _refreshPropertiesOnLoad = true;
            //_propertiesEditor.BuildLayoutOnUpdate();
            _propertiesEditor.BuildLayout();
        }

        /// <inheritdoc />
        public object GetParameter(int index)
        {
            var param = Surface.Parameters[index];
            return param.Value;
        }

        /// <inheritdoc />
        public virtual void SetParameter(int index, object value)
        {
            var param = Surface.Parameters[index];
            param.Value = value;
            _paramValueChange = true;
        }

        /// <inheritdoc />
        public Asset VisjectAsset => Asset;

        /// <inheritdoc />
        public VisjectSurface VisjectSurface => _surface;
    }
}
