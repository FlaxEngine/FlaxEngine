// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.Utilities;

namespace FlaxEditor.States
{
    /// <summary>
    /// In this state engine is building static lighting for the scene. Editing scene and content is blocked.
    /// </summary>
    /// <seealso cref="FlaxEditor.States.EditorState" />
    [HideInEditor]
    public sealed class BuildingLightingState : EditorState
    {
        private bool _wasBuildFinished;

        internal BuildingLightingState(Editor editor)
        : base(editor)
        {
        }

        /// <inheritdoc />
        public override bool CanEditContent => false;

        /// <inheritdoc />
        public override bool IsPerformanceHeavy => true;

        /// <inheritdoc />
        public override string Status => "Baking lighting...";

        /// <inheritdoc />
        public override bool CanEnter()
        {
            return StateMachine.IsEditMode;
        }

        /// <inheritdoc />
        public override bool CanExit(State nextState)
        {
            return _wasBuildFinished;
        }

        /// <inheritdoc />
        public override void OnEnter()
        {
            // Clear flag
            _wasBuildFinished = false;

            // Bind event
            Editor.LightmapsBakeEnd += OnLightmapsBakeEnd;

            // Start building
            Editor.Scene.MarkAllScenesEdited();
            Editor.Internal_BakeLightmaps(false);
        }

        /// <inheritdoc />
        public override void UpdateFPS()
        {
            Time.DrawFPS = 0;
            Time.UpdateFPS = 0;
            Time.PhysicsFPS = 30;
        }

        /// <inheritdoc />
        public override void OnExit(State nextState)
        {
            // Unbind event
            Editor.LightmapsBakeEnd -= OnLightmapsBakeEnd;
        }

        private void OnLightmapsBakeEnd(bool failed)
        {
            Assert.IsTrue(IsActive && !_wasBuildFinished);
            _wasBuildFinished = true;
            Editor.StateMachine.GoToState<EditingSceneState>();
        }
    }
}
