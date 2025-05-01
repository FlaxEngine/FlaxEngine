// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// Describes objects that can own gizmo tools.
    /// </summary>
    [HideInEditor]
    public interface IGizmoOwner
    {
        /// <summary>
        /// Gets the gizmos collection.
        /// </summary>
        FlaxEditor.Viewport.EditorViewport Viewport { get; }

        /// <summary>
        /// Gets the gizmos collection.
        /// </summary>
        GizmosCollection Gizmos { get; }

        /// <summary>
        /// Gets the render task used by the owner to render the scene and the gizmos.
        /// </summary>
        SceneRenderTask RenderTask { get; }

        /// <summary>
        /// Gets a value indicating whether left mouse button is pressed down.
        /// </summary>
        bool IsLeftMouseButtonDown { get; }

        /// <summary>
        /// Gets a value indicating whether right mouse button is pressed down.
        /// </summary>
        bool IsRightMouseButtonDown { get; }

        /// <summary>
        /// Gets a value indicating whether Alt key is pressed down.
        /// </summary>
        bool IsAltKeyDown { get; }

        /// <summary>
        /// Gets a value indicating whether Control key is pressed down.
        /// </summary>
        bool IsControlDown { get; }

        /// <summary>
        /// Gets a value indicating whether snap selected objects to ground (check if user pressed the given input key to call action).
        /// </summary>
        bool SnapToGround { get; }

        /// <summary>
        /// Gets a value indicating whether to use vertex snapping (check if user pressed the given input key to call action).
        /// </summary>
        bool SnapToVertex { get; }

        /// <summary>
        /// Gets the view forward direction.
        /// </summary>
        Float3 ViewDirection { get; }

        /// <summary>
        /// Gets the view position.
        /// </summary>
        Vector3 ViewPosition { get; }

        /// <summary>
        /// Gets the view orientation.
        /// </summary>
        Quaternion ViewOrientation { get; }

        /// <summary>
        /// Gets the view far clipping plane.
        /// </summary>
        float ViewFarPlane { get; }

        /// <summary>
        /// Gets the mouse ray (in world space of the viewport).
        /// </summary>
        Ray MouseRay { get; }

        /// <summary>
        /// Gets the mouse movement delta.
        /// </summary>
        Float2 MouseDelta { get; }

        /// <summary>
        /// Gets a value indicating whether use grid snapping during gizmo operations.
        /// </summary>
        bool UseSnapping { get; }

        /// <summary>
        /// Gets a value indicating whether duplicate objects during gizmo operation (eg. when transforming).
        /// </summary>
        bool UseDuplicate { get; }

        /// <summary>
        /// Gets a <see cref="FlaxEditor.Undo"/> object used by the gizmo owner.
        /// </summary>
        Undo Undo { get; }

        /// <summary>
        /// Gets the root tree node for the scene graph.
        /// </summary>
        SceneGraph.RootNode SceneGraphRoot { get; }

        /// <summary>
        /// Selects the scene objects.
        /// </summary>
        /// <param name="nodes">The nodes to select</param>
        void Select(List<SceneGraph.SceneGraphNode> nodes);

        /// <summary>
        /// Spawns the actor in the viewport hierarchy.
        /// </summary>
        /// <param name="actor">The new actor to spawn.</param>
        void Spawn(Actor actor);

        /// <summary>
        /// Opens the context menu at the current mouse location (using current selection).
        /// </summary>
        void OpenContextMenu();
    }
}
