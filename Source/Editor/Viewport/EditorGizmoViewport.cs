// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEditor.Gizmo;
using FlaxEditor.Viewport.Cameras;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport
{
    /// <summary>
    /// Viewport with free camera and gizmo tools.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.EditorViewport" />
    /// <seealso cref="IGizmoOwner" />
    public class EditorGizmoViewport : EditorViewport, IGizmoOwner
    {
        private UpdateDelegate _update;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorGizmoViewport"/> class.
        /// </summary>
        /// <param name="task">The task.</param>
        /// <param name="undo">The undo.</param>
        /// <param name="sceneGraphRoot">The scene graph root.</param>
        public EditorGizmoViewport(SceneRenderTask task, Undo undo, SceneGraph.RootNode sceneGraphRoot)
        : base(task, new FPSCamera(), true)
        {
            Undo = undo;
            SceneGraphRoot = sceneGraphRoot;
            Gizmos = new GizmosCollection(this);

            SetUpdate(ref _update, OnUpdate);
        }

        private void OnUpdate(float deltaTime)
        {
            for (int i = 0; i < Gizmos.Count; i++)
            {
                Gizmos[i].Update(deltaTime);
            }
        }

        /// <inheritdoc />
        public GizmosCollection Gizmos { get; }

        /// <inheritdoc />
        public SceneRenderTask RenderTask => Task;

        /// <inheritdoc />
        public float ViewFarPlane => FarPlane;

        /// <inheritdoc />
        public bool IsLeftMouseButtonDown => _input.IsMouseLeftDown;

        /// <inheritdoc />
        public bool IsRightMouseButtonDown => _input.IsMouseRightDown;

        /// <inheritdoc />
        public bool IsAltKeyDown => _input.IsAltDown;

        /// <inheritdoc />
        public bool IsControlDown => _input.IsControlDown;

        /// <inheritdoc />
        public bool SnapToGround => Editor.Instance.Options.Options.Input.SnapToGround.Process(Root);

        /// <inheritdoc />
        public Float2 MouseDelta => _mouseDelta * 1000;

        /// <inheritdoc />
        public bool UseSnapping => Root.GetKey(KeyboardKeys.Control);

        /// <inheritdoc />
        public bool UseDuplicate => Root.GetKey(KeyboardKeys.Shift);

        /// <inheritdoc />
        public Undo Undo { get; }

        /// <inheritdoc />
        public SceneGraph.RootNode SceneGraphRoot { get; }

        /// <inheritdoc />
        protected override bool IsControllingMouse => Gizmos.Active?.IsControllingMouse ?? false;

        /// <inheritdoc />
        protected override void AddUpdateCallbacks(RootControl root)
        {
            base.AddUpdateCallbacks(root);

            root.UpdateCallbacksToAdd.Add(_update);
        }

        /// <inheritdoc />
        protected override void RemoveUpdateCallbacks(RootControl root)
        {
            base.RemoveUpdateCallbacks(root);

            root.UpdateCallbacksToRemove.Add(_update);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;

            Gizmos.Clear();

            base.OnDestroy();
        }
    }
}
