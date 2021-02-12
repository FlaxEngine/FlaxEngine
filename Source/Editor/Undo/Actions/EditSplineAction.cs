// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Modules;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;

namespace FlaxEditor.Actions
{
    /// <summary>
    /// Change <see cref="Spline"/> keyframes undo action.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    /// <seealso cref="FlaxEditor.ISceneEditAction" />
    [Serializable]
    public class EditSplineAction : IUndoAction, ISceneEditAction
    {
        [Serialize]
        private Guid _splineId;

        [Serialize]
        private BezierCurve<Transform>.Keyframe[] _before;

        [Serialize]
        private BezierCurve<Transform>.Keyframe[] _after;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditSplineAction"/> class.
        /// </summary>
        /// <param name="spline">The spline.</param>
        /// <param name="before">The spline keyframes state before editing it.</param>
        public EditSplineAction(Spline spline, BezierCurve<Transform>.Keyframe[] before)
        {
            _splineId = spline.ID;
            _before = before;
            _after = (BezierCurve<Transform>.Keyframe[])spline.SplineKeyframes.Clone();
        }

        /// <inheritdoc />
        public string ActionString => "Edit spline keyframes";

        /// <inheritdoc />
        public void Do()
        {
            var spline = FlaxEngine.Object.Find<Spline>(ref _splineId);
            if (spline == null)
                return;
            spline.SplineKeyframes = _after;
            SplineNode.OnSplineEdited(spline);
        }

        /// <inheritdoc />
        public void Undo()
        {
            var spline = FlaxEngine.Object.Find<Spline>(ref _splineId);
            if (spline == null)
                return;
            spline.SplineKeyframes = _before;
            SplineNode.OnSplineEdited(spline);
        }

        /// <inheritdoc />
        public void Dispose()
        {
            _before = _after = null;
        }

        /// <inheritdoc />
        public void MarkSceneEdited(SceneModule sceneModule)
        {
            var spline = FlaxEngine.Object.Find<Spline>(ref _splineId);
            if (spline != null)
                sceneModule.MarkSceneEdited(spline.Scene);
        }
    }
}
