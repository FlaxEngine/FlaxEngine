// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Tools.Terrain
{
    /// <summary>
    /// Carve tab related to terrain editing. Allows to pick a terrain patch and remove it or add new patches. Can be used to modify selected chunk properties.
    /// </summary>
    /// <seealso cref="Tab" />
    [HideInEditor]
    public class EditTab : Tab
    {
        /// <summary>
        /// The parent carve tab.
        /// </summary>
        public readonly CarveTab CarveTab;

        /// <summary>
        /// The related edit terrain gizmo.
        /// </summary>
        public readonly EditTerrainGizmoMode Gizmo;

        private readonly ComboBox _modeComboBox;
        private readonly Label _selectionInfoLabel;
        private readonly Button _deletePatchButton;
        private readonly Button _exportTerrainButton;
        private readonly ContainerControl _chunkProperties;
        private readonly AssetPicker _chunkOverrideMaterial;
        private bool _isUpdatingUI;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditTab"/> class.
        /// </summary>
        /// <param name="tab">The parent tab.</param>
        /// <param name="gizmo">The related gizmo.</param>
        public EditTab(CarveTab tab, EditTerrainGizmoMode gizmo)
        : base("Edit")
        {
            CarveTab = tab;
            Gizmo = gizmo;
            CarveTab.SelectedTerrainChanged += OnSelectionChanged;
            Gizmo.SelectedChunkCoordChanged += OnSelectionChanged;

            // Main panel
            var panel = new Panel(ScrollBars.Both)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this
            };

            // Mode
            var modeLabel = new Label(4, 4, 40, 18)
            {
                HorizontalAlignment = TextAlignment.Near,
                Text = "Mode:",
                Parent = panel,
            };
            _modeComboBox = new ComboBox(modeLabel.Right + 4, 4, 110)
            {
                Parent = panel,
            };
            _modeComboBox.AddItem("Edit Chunk");
            _modeComboBox.AddItem("Add Patch");
            _modeComboBox.AddItem("Remove Patch");
            _modeComboBox.AddItem("Export terrain");
            _modeComboBox.SelectedIndex = 0;
            _modeComboBox.SelectedIndexChanged += (combobox) => Gizmo.EditMode = (EditTerrainGizmoMode.Modes)combobox.SelectedIndex;
            Gizmo.ModeChanged += OnGizmoModeChanged;

            // Info
            _selectionInfoLabel = new Label(modeLabel.X, modeLabel.Bottom + 4, 40, 18 * 3)
            {
                VerticalAlignment = TextAlignment.Near,
                HorizontalAlignment = TextAlignment.Near,
                Parent = panel,
            };

            // Chunk Properties
            _chunkProperties = new Panel(ScrollBars.None)
            {
                Location = new Vector2(_selectionInfoLabel.X, _selectionInfoLabel.Bottom + 4),
                Parent = panel,
            };
            var chunkOverrideMaterialLabel = new Label(0, 0, 90, 64)
            {
                HorizontalAlignment = TextAlignment.Near,
                Text = "Override Material",
                Parent = _chunkProperties,
            };
            _chunkOverrideMaterial = new AssetPicker(new ScriptType(typeof(MaterialBase)), new Vector2(chunkOverrideMaterialLabel.Right + 4, 0))
            {
                Width = 300.0f,
                Parent = _chunkProperties,
            };
            _chunkOverrideMaterial.SelectedItemChanged += OnSelectedChunkOverrideMaterialChanged;
            _chunkProperties.Size = new Vector2(_chunkOverrideMaterial.Right + 4, _chunkOverrideMaterial.Bottom + 4);

            // Delete patch
            _deletePatchButton = new Button(_selectionInfoLabel.X, _selectionInfoLabel.Bottom + 4)
            {
                Text = "Delete Patch",
                Parent = panel,
            };
            _deletePatchButton.Clicked += OnDeletePatchButtonClicked;

            // Export terrain
            _exportTerrainButton = new Button(_selectionInfoLabel.X, _selectionInfoLabel.Bottom + 4)
            {
                Text = "Export terrain",
                Parent = panel,
            };
            _exportTerrainButton.Clicked += OnExportTerrainButtonClicked;

            // Update UI to match the current state
            OnSelectionChanged();
            OnGizmoModeChanged();
        }

        [Serializable]
        private class DeletePatchAction : IUndoAction
        {
            [Serialize]
            private Guid _terrainId;

            [Serialize]
            private Int2 _patchCoord;

            [Serialize]
            private string _data;

            /// <inheritdoc />
            public string ActionString => "Delete terrain patch";

            public DeletePatchAction(FlaxEngine.Terrain terrain, ref Int2 patchCoord)
            {
                if (terrain == null)
                    throw new ArgumentException(nameof(terrain));

                _terrainId = terrain.ID;
                _patchCoord = patchCoord;
                _data = TerrainTools.SerializePatch(terrain, ref patchCoord);
            }

            /// <inheritdoc />
            public void Do()
            {
                var terrain = Object.Find<FlaxEngine.Terrain>(ref _terrainId);
                if (terrain == null)
                {
                    Editor.LogError("Missing terrain actor.");
                    return;
                }

                terrain.GetPatchBounds(terrain.GetPatchIndex(ref _patchCoord), out var patchBounds);
                terrain.RemovePatch(ref _patchCoord);
                OnPatchEdit(terrain, ref patchBounds);
            }

            /// <inheritdoc />
            public void Undo()
            {
                var terrain = Object.Find<FlaxEngine.Terrain>(ref _terrainId);
                if (terrain == null)
                {
                    Editor.LogError("Missing terrain actor.");
                    return;
                }

                terrain.AddPatch(ref _patchCoord);
                TerrainTools.DeserializePatch(terrain, ref _patchCoord, _data);
                terrain.GetPatchBounds(terrain.GetPatchIndex(ref _patchCoord), out var patchBounds);
                OnPatchEdit(terrain, ref patchBounds);
            }

            private void OnPatchEdit(FlaxEngine.Terrain terrain, ref BoundingBox patchBounds)
            {
                Editor.Instance.Scene.MarkSceneEdited(terrain.Scene);

                var editorOptions = Editor.Instance.Options.Options;
                bool isPlayMode = Editor.Instance.StateMachine.IsPlayMode;

                // Auto NavMesh rebuild
                if (!isPlayMode && editorOptions.General.AutoRebuildNavMesh)
                {
                    if (terrain.Scene && terrain.HasStaticFlag(StaticFlags.Navigation))
                    {
                        Navigation.BuildNavMesh(terrain.Scene, patchBounds, editorOptions.General.AutoRebuildNavMeshTimeoutMs);
                    }
                }
            }

            /// <inheritdoc />
            public void Dispose()
            {
                _data = null;
            }
        }

        private void OnDeletePatchButtonClicked()
        {
            if (_isUpdatingUI)
                return;

            var patchCoord = Gizmo.SelectedPatchCoord;
            if (!CarveTab.SelectedTerrain.HasPatch(ref patchCoord))
                return;

            var action = new DeletePatchAction(CarveTab.SelectedTerrain, ref patchCoord);
            action.Do();
            CarveTab.Editor.Undo.AddAction(action);
        }

        [Serializable]
        private class EditChunkMaterialAction : IUndoAction
        {
            [Serialize]
            private Guid _terrainId;

            [Serialize]
            private Int2 _patchCoord;

            [Serialize]
            private Int2 _chunkCoord;

            [Serialize]
            private Guid _beforeMaterial;

            [Serialize]
            private Guid _afterMaterial;

            /// <inheritdoc />
            public string ActionString => "Edit terrain chunk material";

            public EditChunkMaterialAction(FlaxEngine.Terrain terrain, ref Int2 patchCoord, ref Int2 chunkCoord, MaterialBase toSet)
            {
                if (terrain == null)
                    throw new ArgumentException(nameof(terrain));

                _terrainId = terrain.ID;
                _patchCoord = patchCoord;
                _chunkCoord = chunkCoord;
                _beforeMaterial = terrain.GetChunkOverrideMaterial(ref patchCoord, ref chunkCoord)?.ID ?? Guid.Empty;
                _afterMaterial = toSet?.ID ?? Guid.Empty;
            }

            /// <inheritdoc />
            public void Do()
            {
                Set(ref _afterMaterial);
            }

            /// <inheritdoc />
            public void Undo()
            {
                Set(ref _beforeMaterial);
            }

            /// <inheritdoc />
            public void Dispose()
            {
            }

            private void Set(ref Guid id)
            {
                var terrain = Object.Find<FlaxEngine.Terrain>(ref _terrainId);
                if (terrain == null)
                {
                    Editor.LogError("Missing terrain actor.");
                    return;
                }

                terrain.SetChunkOverrideMaterial(ref _patchCoord, ref _chunkCoord, FlaxEngine.Content.LoadAsync<MaterialBase>(id));

                Editor.Instance.Scene.MarkSceneEdited(terrain.Scene);
            }
        }

        private void OnSelectedChunkOverrideMaterialChanged()
        {
            if (_isUpdatingUI)
                return;

            var patchCoord = Gizmo.SelectedPatchCoord;
            var chunkCoord = Gizmo.SelectedChunkCoord;
            var action = new EditChunkMaterialAction(CarveTab.SelectedTerrain, ref patchCoord, ref chunkCoord, _chunkOverrideMaterial.SelectedAsset as MaterialBase);
            action.Do();
            CarveTab.Editor.Undo.AddAction(action);
        }

        private void OnExportTerrainButtonClicked()
        {
            if (_isUpdatingUI)
                return;

            if (FileSystem.ShowBrowseFolderDialog(null, null, "Select the output folder", out var outputFolder))
                return;
            TerrainTools.ExportTerrain(CarveTab.SelectedTerrain, outputFolder);
        }

        private void OnSelectionChanged()
        {
            var terrain = CarveTab.SelectedTerrain;
            if (terrain == null)
            {
                _selectionInfoLabel.Text = "Select a terrain to modify its properties.";
                _chunkProperties.Visible = false;
                _deletePatchButton.Visible = false;
                _exportTerrainButton.Visible = false;
            }
            else
            {
                var patchCoord = Gizmo.SelectedPatchCoord;
                switch (Gizmo.EditMode)
                {
                case EditTerrainGizmoMode.Modes.Edit:
                {
                    var chunkCoord = Gizmo.SelectedChunkCoord;
                    _selectionInfoLabel.Text = string.Format(
                        "Selected terrain: {0}\nPatch: {1}x{2}\nChunk: {3}x{4}",
                        terrain.Name,
                        patchCoord.X, patchCoord.Y,
                        chunkCoord.X, chunkCoord.Y
                    );
                    _chunkProperties.Visible = true;
                    _deletePatchButton.Visible = false;
                    _exportTerrainButton.Visible = false;

                    _isUpdatingUI = true;
                    if (terrain.HasPatch(ref patchCoord))
                    {
                        _chunkOverrideMaterial.SelectedAsset = terrain.GetChunkOverrideMaterial(ref patchCoord, ref chunkCoord);
                        _chunkOverrideMaterial.Enabled = true;
                    }
                    else
                    {
                        _chunkOverrideMaterial.SelectedAsset = null;
                        _chunkOverrideMaterial.Enabled = false;
                    }
                    _isUpdatingUI = false;
                    break;
                }
                case EditTerrainGizmoMode.Modes.Add:
                {
                    if (terrain.HasPatch(ref patchCoord))
                    {
                        _selectionInfoLabel.Text = string.Format(
                            "Selected terrain: {0}\nMove mouse cursor at location without a patch.",
                            terrain.Name
                        );
                    }
                    else
                    {
                        _selectionInfoLabel.Text = string.Format(
                            "Selected terrain: {0}\nPatch to add: {1}x{2}\nTo add a new patch press the left mouse button.",
                            terrain.Name,
                            patchCoord.X, patchCoord.Y
                        );
                    }
                    _chunkProperties.Visible = false;
                    _deletePatchButton.Visible = false;
                    _exportTerrainButton.Visible = false;
                    break;
                }
                case EditTerrainGizmoMode.Modes.Remove:
                {
                    _selectionInfoLabel.Text = string.Format(
                        "Selected terrain: {0}\nPatch: {1}x{2}",
                        terrain.Name,
                        patchCoord.X, patchCoord.Y
                    );
                    _chunkProperties.Visible = false;
                    _deletePatchButton.Visible = true;
                    _exportTerrainButton.Visible = false;
                    break;
                }
                case EditTerrainGizmoMode.Modes.Export:
                {
                    _selectionInfoLabel.Text = string.Format(
                        "Selected terrain: {0}",
                        terrain.Name
                    );
                    _chunkProperties.Visible = false;
                    _deletePatchButton.Visible = false;
                    _exportTerrainButton.Visible = true;
                    break;
                }
                }
            }
        }

        private void OnGizmoModeChanged()
        {
            _modeComboBox.SelectedIndex = (int)Gizmo.EditMode;
            OnSelectionChanged();
        }
    }
}
