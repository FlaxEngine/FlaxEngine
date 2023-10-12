// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport;
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
        /// Gets the Viewport Gizmo is used in
        /// </summary>
        EditorViewport Viewport { get; }

        /// <summary>
        /// Gets the gizmos collection.
        /// </summary>
        GizmosCollection Gizmos { get; }

        /// <summary>
        /// Gets the mouse ray (in world space of the viewport).
        /// </summary>
        Ray MouseRay { get; }

        /// <summary>
        /// Gets the mouse movement delta.
        /// </summary>
        Float2 MouseDelta { get; }

        /// <summary>
        /// Gets a <see cref="T:FlaxEditor.Undo" /> object used by the gizmo owner.
        /// </summary>
        Undo Undo { get; }

        /// <summary>
        /// Gets the root tree node for the scene graph.
        /// </summary>
        RootNode SceneGraphRoot { get; }

        /// <summary>
        /// Gets the view forward direction.
        /// </summary>
        Float3 ViewDirection => Viewport.Camera.Forward;

        /// <summary>
        /// Gets the view position.
        /// </summary>
        Vector3 ViewPosition => Viewport.Camera.Translation;

        /// <summary>
        /// Gets the view orientation.
        /// </summary>
        Quaternion ViewOrientation => Viewport.Camera.Orientation;

        /// <summary>
        /// Gets the render task used by the owner to render the scene and the gizmos.
        /// </summary>
        SceneRenderTask RenderTask => Viewport.Task;

        /// <summary>
        /// Gets a value indicating whether left mouse button is pressed down.
        /// </summary>
        bool IsLeftMouseButtonDown => Viewport.Input.IsMouseLeftDown;

        /// <summary>
        /// Gets a value indicating whether right mouse button is pressed down.
        /// </summary>
        bool IsRightMouseButtonDown => Viewport.Input.IsMouseRightDown;

        /// <summary>
        /// Gets a value indicating whether Alt key is pressed down.
        /// </summary>
        bool IsAltKeyDown => Viewport.Input.IsAltDown;

        /// <summary>
        /// Gets a value indicating whether Control key is pressed down.
        /// </summary>
        bool IsControlDown => Viewport.Input.IsControlDown;

        /// <summary>
        /// Gets a value indicating whether snap selected objects to ground (check if user pressed the given input key to call action).
        /// </summary>
        bool SnapToGround => Editor.Instance.Options.Options.Input.SnapToGround.Process(this.Viewport.Root);

        /// <summary>
        /// Gets the view far clipping plane.
        /// </summary>
        float ViewFarPlane => Viewport.Camera.FarPlane;

        /// <summary>
        /// Gets a value indicating whether use grid snapping during gizmo operations.
        /// </summary>
        bool UseSnapping => Viewport.Root.GetKey(KeyboardKeys.Control);

        /// <summary>
        /// Gets a value indicating whether duplicate objects during gizmo operation (eg. when transforming).
        /// </summary>
        SceneGraph.RootNode SceneGraphRoot { get; }

        /// <summary>
        /// Selects the scene objects.
        /// </summary>
        /// <param name="nodes">The nodes to select</param>
        void Select(List<SceneGraph.SceneGraphNode> nodes);
        bool UseDuplicate => Viewport.Root.GetKey(KeyboardKeys.Shift);
    }
}
