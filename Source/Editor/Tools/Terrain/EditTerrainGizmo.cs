// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Tools.Terrain
{
    /// <summary>
    /// Gizmo for picking terrain chunks and patches. Managed by the <see cref="EditTerrainGizmoMode"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Gizmo.GizmoBase" />
    [HideInEditor]
    public sealed class EditTerrainGizmo : GizmoBase
    {
        private Model _planeModel;
        private MaterialBase _highlightMaterial;
        private MaterialBase _highlightTerrainMaterial;

        /// <summary>
        /// The parent mode.
        /// </summary>
        public readonly EditTerrainGizmoMode Mode;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditTerrainGizmo"/> class.
        /// </summary>
        /// <param name="owner">The owner.</param>
        /// <param name="mode">The mode.</param>
        public EditTerrainGizmo(IGizmoOwner owner, EditTerrainGizmoMode mode)
        : base(owner)
        {
            Mode = mode;
        }

        private void GetAssets()
        {
            if (_planeModel)
                return;

            _planeModel = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Primitives/Plane");
            _highlightTerrainMaterial = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.HighlightTerrainMaterial);
            _highlightMaterial = EditorAssets.Cache.HighlightMaterialInstance;
        }

        private FlaxEngine.Terrain SelectedTerrain
        {
            get
            {
                var sceneEditing = Editor.Instance.SceneEditing;
                var terrainNode = sceneEditing.SelectionCount == 1 ? sceneEditing.Selection[0] as TerrainNode : null;
                return (FlaxEngine.Terrain)terrainNode?.Actor;
            }
        }

        /// <inheritdoc />
        public override void Draw(ref RenderContext renderContext)
        {
            if (!IsActive)
                return;

            var terrain = SelectedTerrain;
            if (!terrain)
                return;

            GetAssets();

            switch (Mode.EditMode)
            {
            case EditTerrainGizmoMode.Modes.Edit:
            {
                // Highlight selected chunk
                var patchCoord = Mode.SelectedPatchCoord;
                if (terrain.HasPatch(ref patchCoord))
                {
                    var chunkCoord = Mode.SelectedChunkCoord;
                    terrain.DrawChunk(ref renderContext, ref patchCoord, ref chunkCoord, _highlightTerrainMaterial);
                }

                break;
            }
            case EditTerrainGizmoMode.Modes.Add:
            {
                // Highlight patch to add location
                var patchCoord = Mode.SelectedPatchCoord;
                if (!terrain.HasPatch(ref patchCoord) && _planeModel)
                {
                    var planeSize = 100.0f;
                    var patchSize = terrain.ChunkSize * FlaxEngine.Terrain.UnitsPerVertex * FlaxEngine.Terrain.PatchEdgeChunksCount;
                    Matrix world = Matrix.RotationX(-Mathf.PiOverTwo) *
                                   Matrix.Scaling(patchSize / planeSize) *
                                   Matrix.Translation(patchSize * (0.5f + patchCoord.X), 0, patchSize * (0.5f + patchCoord.Y)) *
                                   Matrix.Scaling(terrain.Scale) *
                                   Matrix.RotationQuaternion(terrain.Orientation) *
                                   Matrix.Translation(terrain.Position - renderContext.View.Origin);
                    _planeModel.Draw(ref renderContext, _highlightMaterial, ref world);
                }

                break;
            }
            case EditTerrainGizmoMode.Modes.Remove:
            {
                // Highlight selected patch
                var patchCoord = Mode.SelectedPatchCoord;
                if (terrain.HasPatch(ref patchCoord))
                {
                    terrain.DrawPatch(ref renderContext, ref patchCoord, _highlightTerrainMaterial);
                }
                break;
            }
            }
        }

        /// <inheritdoc />
        public override void Update(float dt)
        {
            base.Update(dt);

            if (!IsActive)
                return;

            if (Mode.EditMode == EditTerrainGizmoMode.Modes.Add)
            {
                var sceneEditing = Editor.Instance.SceneEditing;
                var terrainNode = sceneEditing.SelectionCount == 1 ? sceneEditing.Selection[0] as TerrainNode : null;
                if (terrainNode != null)
                {
                    // Check if mouse ray hits any of the terrain patches sides to add a new patch there
                    var mouseRay = Owner.MouseRay;
                    var terrain = (FlaxEngine.Terrain)terrainNode.Actor;
                    var chunkCoord = Int2.Zero;
                    if (!TerrainTools.TryGetPatchCoordToAdd(terrain, mouseRay, out var patchCoord))
                    {
                        // If terrain has no patches TryGetPatchCoordToAdd will always return the default patch to add, otherwise fallback to already used location
                        terrain.GetPatchCoord(0, out patchCoord);
                    }
                    Mode.SetSelectedChunk(ref patchCoord, ref chunkCoord);
                }
            }
        }

        [Serializable]
        private class AddPatchAction : IUndoAction
        {
            [Serialize]
            private Guid _terrainId;

            [Serialize]
            private Int2 _patchCoord;

            /// <inheritdoc />
            public string ActionString => "Add terrain patch";

            public AddPatchAction(FlaxEngine.Terrain terrain, ref Int2 patchCoord)
            {
                if (terrain == null)
                    throw new ArgumentException(nameof(terrain));

                _terrainId = terrain.ID;
                _patchCoord = patchCoord;
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

                terrain.AddPatch(ref _patchCoord);
                if (TerrainTools.InitializePatch(terrain, ref _patchCoord))
                {
                    Editor.LogError("Failed to initialize terrain patch.");
                }
                terrain.GetPatchBounds(terrain.GetPatchIndex(ref _patchCoord), out var patchBounds);
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

                terrain.GetPatchBounds(terrain.GetPatchIndex(ref _patchCoord), out var patchBounds);
                terrain.RemovePatch(ref _patchCoord);
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
            }
        }

        private bool TryAddPatch()
        {
            var terrain = SelectedTerrain;
            if (terrain)
            {
                var patchCoord = Mode.SelectedPatchCoord;
                if (!terrain.HasPatch(ref patchCoord))
                {
                    // Add a new patch (with undo)
                    var action = new AddPatchAction(terrain, ref patchCoord);
                    action.Do();
                    Editor.Instance.Undo.AddAction(action);
                    return true;
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override void Pick()
        {
            if (Mode.EditMode == EditTerrainGizmoMode.Modes.Add && TryAddPatch())
            {
                // Patch added!
            }
            else
            {
                // Get mouse ray and try to hit terrain
                var ray = Owner.MouseRay;
                var view = new Ray(Owner.ViewPosition, Owner.ViewDirection);
                var rayCastFlags = SceneGraphNode.RayCastData.FlagTypes.SkipColliders;
                var hit = Editor.Instance.Scene.Root.RayCast(ref ray, ref view, out _, rayCastFlags) as TerrainNode;

                // Update selection
                var sceneEditing = Editor.Instance.SceneEditing;
                if (hit != null)
                {
                    if (Mode.EditMode != EditTerrainGizmoMode.Modes.Add)
                    {
                        // Perform detailed tracing
                        var terrain = (FlaxEngine.Terrain)hit.Actor;
                        terrain.RayCast(ray, out _, out var patchCoord, out var chunkCoord);
                        Mode.SetSelectedChunk(ref patchCoord, ref chunkCoord);
                    }

                    sceneEditing.Select(hit);
                }
                else
                {
                    sceneEditing.Deselect();
                }
            }
        }
    }
}
