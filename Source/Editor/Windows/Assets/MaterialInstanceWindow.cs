// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Surface;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Material window allows to view and edit <see cref="MaterialInstance"/> asset.
    /// Note: it uses actual asset to modify so changes are visible live in the game/editor preview.
    /// </summary>
    /// <seealso cref="MaterialInstance" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class MaterialInstanceWindow : AssetEditorWindowBase<MaterialInstance>
    {
        private sealed class EditParamOverrideAction : IUndoAction
        {
            public MaterialInstanceWindow Window;
            public string Name;
            public bool Before;

            /// <inheritdoc />
            public string ActionString => "Edit Override";

            private void Set(bool value)
            {
                Window.Asset.GetParameter(Name).IsOverride = value;
            }

            /// <inheritdoc />
            public void Do()
            {
                Set(!Before);
            }

            /// <inheritdoc />
            public void Undo()
            {
                Set(Before);
            }

            /// <inheritdoc />
            public void Dispose()
            {
                Window = null;
            }
        }

        /// <summary>
        /// The material properties proxy object.
        /// </summary>
        [CustomEditor(typeof(ParametersEditor))]
        private sealed class PropertiesProxy
        {
            private MaterialBase _restoreBase;
            private Dictionary<string, object> _restoreParams;

            [EditorDisplay("General"), Tooltip("The base material used to override it's properties")]
            public MaterialBase BaseMaterial
            {
                get => Window?.Asset != null ? Window?.Asset.BaseMaterial : null;
                set
                {
                    var asset = Window?.Asset;
                    if (asset)
                    {
                        if (value == asset)
                        {
                            MessageBox.Show("Cannot use material itself as instance base.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                            return;
                        }
                        asset.BaseMaterial = value;
                        Window._editor.BuildLayoutOnUpdate();
                    }
                }
            }

            /// <summary>
            /// The window reference. Used to handle some special logic.
            /// </summary>
            [NoSerialize, HideInEditor]
            public MaterialInstanceWindow Window;

            /// <summary>
            /// The material parameter values collection. Used to record undo changes.
            /// </summary>
            /// <remarks>
            /// Contains only items with raw values excluding Flax Objects.
            /// </remarks>
            [HideInEditor]
            public object[] Values
            {
                get => Window?.Asset != null ? Window?.Asset.Parameters.Select(x => x.Value).ToArray() : null;
                set
                {
                    if (Window?.Asset == null)
                        return;
                    var parameters = Window?.Asset.Parameters;
                    if (value != null && parameters != null)
                    {
                        if (value.Length != parameters.Length)
                            return;

                        for (int i = 0; i < value.Length; i++)
                        {
                            var p = parameters[i].Value;
                            if (p is FlaxEngine.Object || p == null)
                                continue;

                            parameters[i].Value = value[i];
                        }
                    }
                }
            }

            /// <summary>
            /// The material parameter values collection. Used to record undo changes.
            /// </summary>
            /// <remarks>
            /// Contains only items with references to Flax Objects identified by ID.
            /// </remarks>
            [HideInEditor]
            public FlaxEngine.Object[] ValuesRef
            {
                get => Window?.Asset != null ? Window?.Asset.Parameters.Select(x => x.Value as FlaxEngine.Object).ToArray() : null;
                set
                {
                    if (Window?.Asset == null)
                        return;
                    var parameters = Window?.Asset?.Parameters;
                    if (value != null && parameters != null)
                    {
                        if (value.Length != parameters.Length)
                            return;

                        for (int i = 0; i < value.Length; i++)
                        {
                            var p = parameters[i].Value;
                            if (!(p is FlaxEngine.Object || p == null))
                                continue;

                            parameters[i].Value = value[i];
                        }
                    }
                }
            }

            /// <summary>
            /// Gathers parameters from the specified material.
            /// </summary>
            /// <param name="materialWin">The material window.</param>
            public void OnLoad(MaterialInstanceWindow materialWin)
            {
                // Link
                Window = materialWin;

                // Prepare restore data
                PeekState();
            }

            /// <summary>
            /// Records the current state to restore it on DiscardChanges.
            /// </summary>
            public void PeekState()
            {
                if (Window == null)
                    return;

                var material = Window.Asset;
                _restoreBase = material.BaseMaterial;
                var parameters = material.Parameters;
                _restoreParams = new Dictionary<string, object>();
                for (int i = 0; i < parameters.Length; i++)
                    _restoreParams[parameters[i].Name] = parameters[i].Value;
            }

            /// <summary>
            /// On discard changes
            /// </summary>
            public void DiscardChanges()
            {
                if (Window == null)
                    return;

                var material = Window.Asset;
                material.BaseMaterial = _restoreBase;
                var parameters = material.Parameters;
                for (int i = 0; i < parameters.Length; i++)
                {
                    var p = parameters[i];
                    if (p.IsPublic && _restoreParams.TryGetValue(p.Name, out var value))
                    {
                        p.Value = value;
                    }
                }
            }

            /// <summary>
            /// Clears temporary data.
            /// </summary>
            public void OnClean()
            {
                // Unlink
                Window = null;
            }
        }

        /// <summary>
        /// Custom editor for editing material parameters collection.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.CustomEditor" />
        public class ParametersEditor : GenericEditor
        {
            private SurfaceUtils.GraphParameterData[] _graphParameters;

            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                // Prepare
                var proxy = (PropertiesProxy)Values[0];
                var materialInstance = proxy.Window?.Asset;
                if (materialInstance == null)
                {
                    layout.Label("No parameters");
                    return;
                }
                if (!materialInstance.IsLoaded || (materialInstance.BaseMaterial && !materialInstance.BaseMaterial.IsLoaded))
                {
                    layout.Label("Loading...", TextAlignment.Center);
                    return;
                }
                var parameters = materialInstance.Parameters;

                base.Initialize(layout);

                if (parameters.Length == 0)
                    return;

                var parametersGroup = SurfaceUtils.InitGraphParametersGroup(layout);
                var settingButton = parametersGroup.AddSettingsButton();
                settingButton.Clicked += (image, button) => OnSettingsButtonClicked(image, button, proxy.Window);
                var baseMaterial = materialInstance.BaseMaterial;
                var material = baseMaterial;
                if (material)
                {
                    while (material is MaterialInstance instance)
                        material = instance.BaseMaterial;
                }

                _graphParameters = SurfaceUtils.InitGraphParameters(parameters, (Material)material);
                SurfaceUtils.DisplayGraphParameters(parametersGroup, _graphParameters,
                                                    (instance, parameter, tag) =>
                                                    {
                                                        // Get material parameter
                                                        var p = (MaterialParameter)tag;
                                                        var proxyEx = (PropertiesProxy)instance;
                                                        var array = proxyEx.Window.Asset.Parameters;
                                                        if (array == null || !array.Contains(p))
                                                            throw new TargetException("Material parameters collection has been changed.");
                                                        return p.Value;
                                                    },
                                                    (instance, value, parameter, tag) =>
                                                    {
                                                        // Set material parameter and surface parameter
                                                        var p = (MaterialParameter)tag;
                                                        var proxyEx = (PropertiesProxy)instance;
                                                        p.Value = value;
                                                        proxyEx.Window._paramValueChange = true;
                                                    },
                                                    Values,
                                                    null,
                                                    (LayoutElementsContainer itemLayout, ValueContainer valueContainer, ref SurfaceUtils.GraphParameterData e) =>
                                                    {
                                                        var p = (MaterialParameter)e.Tag;

                                                        // Try to get default value (from the base material)
                                                        var pBase = baseMaterial != null ? baseMaterial.GetParameter(p.Name) : null;
                                                        if (pBase != null && pBase.ParameterType == p.ParameterType)
                                                        {
                                                            valueContainer.SetDefaultValue(pBase.Value);
                                                        }

                                                        // Add label with checkbox for parameter value override
                                                        var label = new CheckablePropertyNameLabel(e.DisplayName);
                                                        label.CheckBox.Checked = p.IsOverride;
                                                        label.CheckBox.Tag = new KeyValuePair<PropertiesProxy, MaterialParameter>(proxy.Window._properties, p);
                                                        label.CheckChanged += nameLabel =>
                                                        {
                                                            var pair = (KeyValuePair<PropertiesProxy, MaterialParameter>)nameLabel.CheckBox.Tag;
                                                            var proxyEx = pair.Key;
                                                            var pEx = pair.Value;
                                                            pEx.IsOverride = nameLabel.CheckBox.Checked;
                                                            proxyEx.Window._undo.AddAction(new EditParamOverrideAction
                                                            {
                                                                Window = proxyEx.Window,
                                                                Name = pEx.Name,
                                                                Before = !nameLabel.CheckBox.Checked,
                                                            });
                                                        };
                                                        itemLayout.Property(label, valueContainer, null, e.Tooltip?.Text);
                                                    });
            }
            
             private void OnSettingsButtonClicked(Image image, MouseButton mouseButton, MaterialInstanceWindow window)
            {
                if (mouseButton != MouseButton.Left)
                    return;

                var cm = new ContextMenu();
                if (window != null)
                    cm.AddButton("Revert All Parameters", window.OnRevertAllParameters).TooltipText = "Reverts all the overridden parameters to the default values.";
                cm.AddButton("Override All Parameters", OnOverrideAll).TooltipText = "Checks all parameters overrides.";
                cm.AddButton("Remove Parameter Overrides", OnRemoveOverrides).TooltipText = "Unchecks all overrides for parameters.";
                cm.Show(image, image.Size);
            }

            private void OnRemoveOverrides()
            {
                OnSetOverrides(false);
            }

            private void OnOverrideAll()
            {
                OnSetOverrides(true);
            }

            private void OnSetOverrides(bool isOverride)
            {
                var proxy = (PropertiesProxy)Values[0];
                var undoActions = new List<IUndoAction>();
                foreach (var graphParameter in _graphParameters)
                {
                    var p = (MaterialParameter)graphParameter.Tag;
                    if (!p.IsPublic || p.IsOverride == isOverride)
                        continue;
                    p.IsOverride = isOverride;
                    undoActions.Add(new EditParamOverrideAction
                    {
                        Window = proxy.Window,
                        Name = p.Name,
                        Before = !isOverride,
                    });
                }
                if (undoActions.Count == 0)
                    return;
                proxy.Window._undo.AddAction(new MultiUndoAction(undoActions));
                proxy.Window.MarkAsEdited();
                Presenter.BuildLayoutOnUpdate();
            }
        }

        private readonly SplitPanel _split;
        private readonly MaterialPreview _preview;
        private readonly ToolStripButton _saveButton;
        private readonly ToolStripButton _undoButton;
        private readonly ToolStripButton _redoButton;

        private readonly CustomEditorPresenter _editor;
        private readonly Undo _undo;
        private readonly PropertiesProxy _properties;
        private bool _isWaitingForLoad;
        internal bool _paramValueChange;

        /// <inheritdoc />
        public MaterialInstanceWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnAction;

            // Toolstrip
            _saveButton = _toolstrip.AddButton(Editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);
            _toolstrip.AddSeparator();
            _undoButton = _toolstrip.AddButton(Editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip("Undo", ref inputOptions.Undo);
            _redoButton = _toolstrip.AddButton(Editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip("Redo", ref inputOptions.Redo);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/graphics/materials/instanced-materials/index.html")).LinkTooltip("See documentation to learn more");

            // Split Panel
            _split = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.5f,
                Parent = this
            };

            // Material preview
            _preview = new MaterialPreview(true)
            {
                Parent = _split.Panel1
            };

            // Material properties editor
            _editor = new CustomEditorPresenter(_undo, "Loading...");
            _editor.Panel.Parent = _split.Panel2;
            _properties = new PropertiesProxy();
            _editor.Modified += OnMaterialPropertyEdited;

            // Setup input actions
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
        }

        private void OnRevertAllParameters()
        {
            var baseMaterial = Asset.BaseMaterial;
            if (!baseMaterial)
                return;
            var parameters = Asset.Parameters;
            var actions = new List<IUndoAction>();
            for (var i = 0; i < parameters.Length; i++)
            {
                var p = parameters[i];
                if (p.IsOverride)
                {
                    p.IsOverride = false;
                    actions.Add(new EditParamOverrideAction
                    {
                        Window = this,
                        Name = p.Name,
                        Before = true,
                    });
                }
            }
            using (new UndoMultiBlock(_undo, _editor.Selection, "Revert all the parameters to the default values", new MultiUndoAction(actions)))
            {
                for (var i = 0; i < parameters.Length; i++)
                {
                    var p = parameters[i];
                    var pBase = baseMaterial.GetParameter(p.Name);
                    if (pBase != null && pBase.ParameterType == p.ParameterType)
                        p.Value = pBase.Value;
                }
            }
            _editor.BuildLayoutOnUpdate();
        }

        private void OnAction(IUndoAction action)
        {
            _paramValueChange = false;
            MarkAsEdited();
            UpdateToolstrip();
        }

        private void OnUndoRedo(IUndoAction action)
        {
            _paramValueChange = false;
            MarkAsEdited();
            UpdateToolstrip();
            _editor.BuildLayoutOnUpdate();
        }

        private void OnMaterialPropertyEdited()
        {
            _paramValueChange = false;
            //MarkAsEdited();
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            if (Asset.Save())
            {
                Editor.LogError("Cannot save asset.");
                return;
            }

            _properties.PeekState();
            ClearEditedFlag();
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
            _properties.OnClean();
            _preview.Material = null;
            _isWaitingForLoad = false;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.Material = _asset;
            _isWaitingForLoad = true;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        protected override void OnClose()
        {
            if (Asset)
            {
                // Discard unsaved changes
                _properties.DiscardChanges();
            }

            // Cleanup
            _undo.Clear();

            base.OnClose();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (_isWaitingForLoad && _asset.IsLoaded && (_asset.BaseMaterial == null || _asset.BaseMaterial.IsLoaded))
            {
                _isWaitingForLoad = false;
                _properties.OnLoad(this);
                _editor.Select(_properties);
                ClearEditedFlag();
                _undo.Clear();
                _editor.BuildLayout();
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            LayoutSerializeSplitter(writer, "Split", _split);
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            LayoutDeserializeSplitter(node, "Split", _split);
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split.SplitterValue = 0.5f;
        }
    }
}
