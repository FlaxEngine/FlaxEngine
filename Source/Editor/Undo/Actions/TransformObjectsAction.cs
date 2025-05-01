// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Modules;
using FlaxEditor.SceneGraph;
using FlaxEngine;

namespace FlaxEditor
{
    /// <summary>
    /// Implementation of <see cref="IUndoAction"/> used to transform a selection of <see cref="SceneGraphNode"/>.
    /// The same logic could be achieved using <see cref="UndoMultiBlock"/> but it would be slower.
    /// Since we use this kind of action very ofter (for <see cref="FlaxEditor.Gizmo.TransformGizmo"/> operations) it's better to provide faster implementation.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    [Serializable]
    public sealed class TransformObjectsAction : UndoActionBase<TransformObjectsAction.DataStorage>, ISceneEditAction
    {
        /// <summary>
        /// The undo data.
        /// </summary>
        [Serializable]
        public struct DataStorage
        {
            /// <summary>
            /// The scene of the selected objects.
            /// </summary>
            public Scene Scene;

            /// <summary>
            /// The selection pool.
            /// </summary>
            public SceneGraphNode[] Selection;

            /// <summary>
            /// The 'before' state.
            /// </summary>
            public Transform[] Before;

            /// <summary>
            /// The 'after' state.
            /// </summary>
            public Transform[] After;

            /// <summary>
            /// The cached bounding box that contains all selected items in 'before' state.
            /// </summary>
            public BoundingBox BeforeBounds;

            /// <summary>
            /// The cached bounding box that contains all selected items in 'after' state.
            /// </summary>
            public BoundingBox AfterBounds;

            /// <summary>
            /// True if navigation system has been modified during editing the selected objects (navmesh auto-rebuild is required).
            /// </summary>
            public bool NavigationDirty;
        }

        internal TransformObjectsAction(List<SceneGraphNode> selection, List<Transform> before, ref BoundingBox boundsBefore, bool navigationDirty)
        {
            var after = Utilities.Utils.GetTransformsAndBounds(selection, out var afterBounds);

            // TODO: support moving objects from more than one scene
            var scene = selection[0].ParentScene?.Scene;

            var data = new DataStorage
            {
                Scene = scene,
                Selection = selection.ToArray(),
                After = after,
                Before = before.ToArray(),
                BeforeBounds = boundsBefore,
                AfterBounds = afterBounds,
                NavigationDirty = navigationDirty,
            };
            Data = data;

            InvalidateBounds(ref data);
        }

        /// <inheritdoc />
        public override string ActionString => "Transform object(s)";

        /// <inheritdoc />
        public override void Do()
        {
            var data = Data;
            for (int i = 0; i < data.Selection.Length; i++)
            {
                data.Selection[i].Transform = data.After[i];
            }
            InvalidateBounds(ref data);
        }

        /// <inheritdoc />
        public override void Undo()
        {
            var data = Data;
            for (int i = 0; i < data.Selection.Length; i++)
            {
                data.Selection[i].Transform = data.Before[i];
            }
            InvalidateBounds(ref data);
        }

        private void InvalidateBounds(ref DataStorage data)
        {
            if (!data.NavigationDirty)
                return;

            var editor = Editor.Instance;
            bool isPlayMode = editor.StateMachine.IsPlayMode;
            var options = editor.Options.Options;

            // Auto NavMesh rebuild
            if (!isPlayMode && options.General.AutoRebuildNavMesh && data.Scene != null)
            {
                // Handle simple case where objects were moved just a little and use one navmesh build request to improve performance
                if (data.BeforeBounds.Intersects(ref data.AfterBounds))
                {
                    Navigation.BuildNavMesh(data.Scene, BoundingBox.Merge(data.BeforeBounds, data.AfterBounds), options.General.AutoRebuildNavMeshTimeoutMs);
                }
                else
                {
                    Navigation.BuildNavMesh(data.Scene, data.BeforeBounds, options.General.AutoRebuildNavMeshTimeoutMs);
                    Navigation.BuildNavMesh(data.Scene, data.AfterBounds, options.General.AutoRebuildNavMeshTimeoutMs);
                }
            }
        }

        void ISceneEditAction.MarkSceneEdited(SceneModule sceneModule)
        {
            var data = Data;
            for (int i = 0; i < data.Selection.Length; i++)
            {
                sceneModule.MarkSceneEdited(data.Selection[i].ParentScene);
            }
        }
    }
}
