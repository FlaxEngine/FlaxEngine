// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.States
{
    /// <summary>
    /// In this state engine is building scenes data such as static lighting, navigation mesh and reflection probes. Editing scene and content is blocked.
    /// </summary>
    /// <seealso cref="FlaxEditor.States.EditorState" />
    [HideInEditor]
    public sealed class BuildingScenesState : EditorState
    {
        private sealed class SubStateMachine : StateMachine
        {
            protected override void SwitchState(State nextState)
            {
                if (CurrentState != null && nextState != null)
                    Editor.Log($"Changing scenes build state from {CurrentState} to {nextState}");

                base.SwitchState(nextState);
            }
        }

        private abstract class SubState : State
        {
            public virtual void Update()
            {
            }

            public virtual void Cancel()
            {
                StateMachine.GoToState<EndState>();
            }
        }

        private sealed class BeginState : SubState
        {
        }

        private sealed class SetupState : SubState
        {
            public override void OnEnter()
            {
                var editor = Editor.Instance;
                foreach (var scene in Level.Scenes)
                {
                    scene.ClearLightmaps();
                    editor.Scene.MarkSceneEdited(scene);
                }
                StateMachine.GoToState<CSGState>();
            }
        }

        private sealed class CSGState : SubState
        {
            public override void OnEnter()
            {
                foreach (var scene in Level.Scenes)
                {
                    scene.BuildCSG(0);
                }
            }

            public override void Update()
            {
                if (!Editor.Internal_GetIsCSGActive())
                    StateMachine.GoToState<EnvProbesNoGIState>();
            }
        }


        private class EnvProbesNoGIState : SubState
        {
            public override void OnEnter()
            {
                Editor.Instance.Scene.ExecuteOnGraph(node =>
                {
                    if (node is EnvironmentProbeNode envProbeNode && envProbeNode.IsActive && envProbeNode.Actor is EnvironmentProbe envProbe)
                    {
                        envProbe.Bake();
                    }
                    else if (node is SkyLightNode skyLightNode && skyLightNode.IsActive && skyLightNode.Actor is SkyLight skyLight && skyLight.Mode == SkyLight.Modes.CaptureScene)
                    {
                        skyLight.Bake();
                    }
                    return node.IsActive;
                });
            }

            public override void Update()
            {
                if (!Editor.Instance.ProgressReporting.BakeEnvProbes.IsActive)
                    StateMachine.GoToState<StaticLightingState>();
            }
        }

        private sealed class StaticLightingState : SubState
        {
            public override void OnEnter()
            {
                Editor.LightmapsBakeEnd += OnLightmapsBakeEnd;

                if (Progress.Handlers.BakeLightmapsProgress.CanBake)
                    Editor.Internal_BakeLightmaps(false);
                else
                    OnLightmapsBakeEnd(false);
            }

            /// <inheritdoc />
            public override void Cancel()
            {
                Editor.Internal_BakeLightmaps(true);

                base.Cancel();
            }

            public override void OnExit(State nextState)
            {
                Editor.LightmapsBakeEnd -= OnLightmapsBakeEnd;
            }

            private void OnLightmapsBakeEnd(bool failed)
            {
                StateMachine.GoToState<EnvProbesWithGIState>();
            }
        }

        private sealed class EnvProbesWithGIState : EnvProbesNoGIState
        {
            public override void Update()
            {
                if (!Editor.Instance.ProgressReporting.BakeEnvProbes.IsActive)
                    StateMachine.GoToState<NavMeshState>();
            }
        }

        private sealed class NavMeshState : SubState
        {
            public override void OnEnter()
            {
                foreach (var scene in Level.Scenes)
                {
                    Navigation.BuildNavMesh(scene, 0);
                }
            }

            public override void Update()
            {
                if (!Navigation.IsBuildingNavMesh)
                    StateMachine.GoToState<EndState>();
            }
        }

        private sealed class EndState : SubState
        {
            public override void OnEnter()
            {
                Editor.Instance.StateMachine.GoToState<EditingSceneState>();
            }
        }

        private SubStateMachine _stateMachine;
        private DateTime _startTime;

        internal BuildingScenesState(Editor editor)
        : base(editor)
        {
            _stateMachine = new SubStateMachine();
            _stateMachine.AddState(new BeginState());
            _stateMachine.AddState(new SetupState());
            _stateMachine.AddState(new CSGState());
            _stateMachine.AddState(new EnvProbesNoGIState());
            _stateMachine.AddState(new StaticLightingState());
            _stateMachine.AddState(new EnvProbesWithGIState());
            _stateMachine.AddState(new NavMeshState());
            _stateMachine.AddState(new EndState());
            _stateMachine.GoToState<BeginState>();
        }

        /// <summary>
        /// Cancels the build.
        /// </summary>
        public void Cancel()
        {
            ((SubState)_stateMachine.CurrentState).Cancel();
        }

        /// <inheritdoc />
        public override bool CanEditContent => false;

        /// <inheritdoc />
        public override bool IsPerformanceHeavy => true;

        /// <inheritdoc />
        public override string Status => "Building scenes...";

        /// <inheritdoc />
        public override bool CanEnter()
        {
            return StateMachine.IsEditMode;
        }

        /// <inheritdoc />
        public override bool CanExit(State nextState)
        {
            return _stateMachine.CurrentState is EndState;
        }

        /// <inheritdoc />
        public override void OnEnter()
        {
            Editor.Log("Starting scenes build...");
            _startTime = DateTime.Now;
            _stateMachine.GoToState<SetupState>();
        }

        /// <inheritdoc />
        public override void Update()
        {
            ((SubState)_stateMachine.CurrentState).Update();
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
            var time = DateTime.Now - _startTime;
            Editor.Log($"Scenes building ended in {Utils.RoundTo1DecimalPlace((float)time.TotalSeconds)}s");
        }
    }
}
