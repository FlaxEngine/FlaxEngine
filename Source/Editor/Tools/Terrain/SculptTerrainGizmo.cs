// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.Tools.Terrain.Sculpt;
using FlaxEngine;

namespace FlaxEditor.Tools.Terrain
{
    /// <summary>
    /// Gizmo for carving terrain. Managed by the <see cref="SculptTerrainGizmoMode"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Gizmo.GizmoBase" />
    [HideInEditor]
    public sealed class SculptTerrainGizmo : GizmoBase
    {
        private FlaxEngine.Terrain _paintTerrain;
        private Ray _prevRay;

        /// <summary>
        /// The parent mode.
        /// </summary>
        public readonly SculptTerrainGizmoMode Mode;

        /// <summary>
        /// Gets a value indicating whether gizmo tool is painting the terrain heightmap.
        /// </summary>
        public bool IsPainting => _paintTerrain != null;

        /// <summary>
        /// Occurs when terrain paint has been started.
        /// </summary>
        public event Action PaintStarted;

        /// <summary>
        /// Occurs when terrain paint has been ended.
        /// </summary>
        public event Action PaintEnded;

        /// <summary>
        /// Initializes a new instance of the <see cref="SculptTerrainGizmo"/> class.
        /// </summary>
        /// <param name="owner">The owner.</param>
        /// <param name="mode">The mode.</param>
        public SculptTerrainGizmo(IGizmoOwner owner, SculptTerrainGizmoMode mode)
        : base(owner)
        {
            Mode = mode;
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

            if (Mode.HasValidHit)
            {
                var brushPosition = Mode.CursorPosition;
                var brushColor = new Color(1.0f, 0.85f, 0.0f); // TODO: expose to editor options
                var brushMaterial = Mode.CurrentBrush.GetBrushMaterial(ref renderContext, ref brushPosition, ref brushColor);
                if (!brushMaterial)
                    return;

                for (int i = 0; i < Mode.ChunksUnderCursor.Count; i++)
                {
                    var chunk = Mode.ChunksUnderCursor[i];
                    terrain.DrawChunk(ref renderContext, ref chunk.PatchCoord, ref chunk.ChunkCoord, brushMaterial);
                }
            }
        }

        /// <summary>
        /// Called to start terrain painting
        /// </summary>
        /// <param name="terrain">The terrain.</param>
        private void PaintStart(FlaxEngine.Terrain terrain)
        {
            // Skip if already is painting
            if (IsPainting)
                return;

            _paintTerrain = terrain;
            PaintStarted?.Invoke();
        }

        /// <summary>
        /// Called to update terrain painting logic.
        /// </summary>
        /// <param name="dt">The delta time (in seconds).</param>
        private void PaintUpdate(float dt)
        {
            // Skip if is not painting
            if (!IsPainting)
                return;

            // Edit the terrain
            Profiler.BeginEvent("Edit Terrain");
            var options = new Mode.Options
            {
                Strength = 1.0f,
                DeltaTime = dt,
                Invert = Owner.IsControlDown
            };
            Mode.CurrentMode.Apply(Mode.CurrentBrush, ref options, Mode, _paintTerrain);
            Profiler.EndEvent();
        }

        /// <summary>
        /// Called to end terrain painting.
        /// </summary>
        private void PaintEnd()
        {
            // Skip if nothing was painted
            if (!IsPainting)
                return;

            _paintTerrain = null;
            PaintEnded?.Invoke();
        }

        /// <inheritdoc />
        public override bool IsControllingMouse => IsPainting;

        /// <inheritdoc />
        public override void Update(float dt)
        {
            base.Update(dt);

            // Check if gizmo is not active
            if (!IsActive)
            {
                PaintEnd();
                return;
            }

            // Check if no terrain is selected
            var terrain = SelectedTerrain;
            if (!terrain)
            {
                PaintEnd();
                return;
            }

            // Increase or decrease brush size with scroll
            if (Input.GetKey(KeyboardKeys.Shift) && !Input.GetMouseButton(MouseButton.Right))
            {
                Mode.CurrentBrush.Size += dt * Mode.CurrentBrush.Size * Input.Mouse.ScrollDelta * 5f;
            }

            // Check if selected terrain was changed during painting
            if (terrain != _paintTerrain && IsPainting)
            {
                PaintEnd();
            }

            // Special case if user is sculpting terrain and mouse is not moving then freeze the brush location to help painting vertical tip objects
            var mouseRay = Owner.MouseRay;
            if (IsPainting && _prevRay == mouseRay)
            {
                // Freeze cursor
            }
            // Perform detailed tracing to find cursor location on the terrain
            else if (terrain.RayCast(mouseRay, out var closest, out var patchCoord, out var chunkCoord))
            {
                var hitLocation = mouseRay.GetPoint(closest);
                Mode.SetCursor(ref hitLocation);
            }
            // No hit
            else
            {
                Mode.ClearCursor();
            }
            _prevRay = mouseRay;

            // Handle painting
            if (Owner.IsLeftMouseButtonDown)
                PaintStart(terrain);
            else
                PaintEnd();
            PaintUpdate(dt);
        }

        /// <inheritdoc />
        public override void Pick()
        {
            // Get mouse ray and try to hit terrain
            var ray = Owner.MouseRay;
            var view = new Ray(Owner.ViewPosition, Owner.ViewDirection);
            var rayCastFlags = SceneGraphNode.RayCastData.FlagTypes.SkipColliders;
            var hit = Editor.Instance.Scene.Root.RayCast(ref ray, ref view, out _, rayCastFlags);

            // Update selection
            var sceneEditing = Editor.Instance.SceneEditing;
            if (hit is TerrainNode)
                sceneEditing.Select(hit);
        }
    }
}
