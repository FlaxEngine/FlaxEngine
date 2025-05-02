// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Options;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using FlaxEditor.Utilities;
using FlaxEngine.Utilities;
using Utils = FlaxEngine.Utils;

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
            public int ActionIndex = -1;
            public readonly List<GeneralOptions.BuildAction> Actions = new List<GeneralOptions.BuildAction>();

            protected override void SwitchState(State nextState)
            {
                if (CurrentState != null && nextState != null)
                    Editor.Log($"Changing scenes build state from {CurrentState} to {nextState}");

                base.SwitchState(nextState);
            }
        }

        private abstract class SubState : State
        {
            public virtual bool DirtyScenes => true;

            public virtual bool CanReloadScripts => false;

            public virtual void Before()
            {
            }

            public virtual void Update()
            {
            }

            public virtual void Done()
            {
                var stateMachine = (SubStateMachine)StateMachine;
                stateMachine.ActionIndex++;
                if (stateMachine.ActionIndex < stateMachine.Actions.Count)
                {
                    var action = stateMachine.Actions[stateMachine.ActionIndex];
                    var state = stateMachine.States.FirstOrDefault(x => x is ActionState a && a.Action == action);
                    if (state != null)
                    {
                        StateMachine.GoToState(state);
                    }
                    else
                    {
                        Editor.LogError($"Missing or invalid build scene action {action}.");
                    }
                    return;
                }

                StateMachine.GoToState<EndState>();
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
                var stateMachine = (SubStateMachine)StateMachine;
                var scenesDirty = false;
                foreach (var state in stateMachine.States)
                {
                    ((SubState)state).Before();
                    scenesDirty |= ((SubState)state).DirtyScenes;
                }
                if (scenesDirty)
                {
                    foreach (var scene in Level.Scenes)
                        Editor.Instance.Scene.MarkSceneEdited(scene);
                }
                Done();
            }
        }

        private abstract class ActionState : SubState
        {
            public abstract GeneralOptions.BuildAction Action { get; }
        }

        private sealed class CSGState : ActionState
        {
            public override GeneralOptions.BuildAction Action => GeneralOptions.BuildAction.CSG;

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
                    Done();
            }
        }

        private class EnvProbesState : ActionState
        {
            public override GeneralOptions.BuildAction Action => GeneralOptions.BuildAction.EnvProbes;

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
                    Done();
            }
        }

        private sealed class StaticLightingState : ActionState
        {
            public override GeneralOptions.BuildAction Action => GeneralOptions.BuildAction.StaticLighting;

            public override void Before()
            {
                foreach (var scene in Level.Scenes)
                    scene.ClearLightmaps();
            }

            public override void OnEnter()
            {
                Editor.LightmapsBakeEnd += OnLightmapsBakeEnd;

                if (Progress.Handlers.BakeLightmapsProgress.CanBake)
                    Editor.Internal_BakeLightmaps(false);
                else
                    OnLightmapsBakeEnd(false);
            }

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
                Done();
            }
        }

        private sealed class NavMeshState : ActionState
        {
            public override GeneralOptions.BuildAction Action => GeneralOptions.BuildAction.NavMesh;

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
                    Done();
            }
        }

        private sealed class CompileScriptsState : ActionState
        {
            private bool _compiled, _reloaded;

            public override GeneralOptions.BuildAction Action => GeneralOptions.BuildAction.CompileScripts;

            public override bool DirtyScenes => false;

            public override bool CanReloadScripts => true;

            public override void OnEnter()
            {
                _compiled = _reloaded = false;
                ScriptsBuilder.Compile();

                ScriptsBuilder.CompilationSuccess += OnCompilationSuccess;
                ScriptsBuilder.CompilationFailed += OnCompilationFailed;
                ScriptsBuilder.ScriptsReloadEnd += OnScriptsReloadEnd;
            }

            public override void OnExit(State nextState)
            {
                ScriptsBuilder.CompilationSuccess -= OnCompilationSuccess;
                ScriptsBuilder.CompilationFailed -= OnCompilationFailed;
                ScriptsBuilder.ScriptsReloadEnd -= OnScriptsReloadEnd;

                base.OnExit(nextState);
            }

            private void OnCompilationSuccess()
            {
                _compiled = true;
            }

            private void OnCompilationFailed()
            {
                Cancel();
            }

            private void OnScriptsReloadEnd()
            {
                _reloaded = true;
            }

            public override void Update()
            {
                if (_compiled && _reloaded)
                    Done();
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
            _stateMachine.AddState(new EnvProbesState());
            _stateMachine.AddState(new StaticLightingState());
            _stateMachine.AddState(new NavMeshState());
            _stateMachine.AddState(new CompileScriptsState());
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
        public override bool CanReloadScripts => ((SubState)_stateMachine.CurrentState).CanReloadScripts;

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
            _stateMachine.ActionIndex = -1;
            _stateMachine.Actions.Clear();
            var actions = (GeneralOptions.BuildAction[])Editor.Options.Options.General.BuildActions?.Clone();
            if (actions != null)
                _stateMachine.Actions.AddRange(actions);
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
