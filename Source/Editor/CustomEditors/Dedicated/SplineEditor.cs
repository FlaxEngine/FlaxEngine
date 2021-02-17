// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.Actions;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="Spline"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(Spline)), DefaultEditor]
    public class SplineEditor : ActorEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (Values.HasDifferentTypes == false)
            {
                layout.Space(10);
                var grid = layout.CustomContainer<UniformGridPanel>();
                grid.CustomControl.SlotsHorizontally = 2;
                grid.CustomControl.SlotsVertically = 1;
                grid.Button("Set Linear Tangents").Button.Clicked += OnSetTangentsLinear;
                grid.Button("Set Smooth Tangents").Button.Clicked += OnSetTangentsSmooth;
            }
        }

        private void OnSetTangentsLinear()
        {
            var enableUndo = Presenter.Undo != null && Presenter.Undo.Enabled;
            for (int i = 0; i < Values.Count; i++)
            {
                if (Values[i] is Spline spline)
                {
                    var before = enableUndo ? (BezierCurve<Transform>.Keyframe[])spline.SplineKeyframes.Clone() : null;
                    spline.SetTangentsLinear();
                    if (enableUndo)
                        Presenter.Undo.AddAction(new EditSplineAction(spline, before));
                    SplineNode.OnSplineEdited(spline);
                    Editor.Instance.Scene.MarkSceneEdited(spline.Scene);
                }
            }
        }

        private void OnSetTangentsSmooth()
        {
            var enableUndo = Presenter.Undo != null && Presenter.Undo.Enabled;
            for (int i = 0; i < Values.Count; i++)
            {
                if (Values[i] is Spline spline)
                {
                    var before = enableUndo ? (BezierCurve<Transform>.Keyframe[])spline.SplineKeyframes.Clone() : null;
                    spline.SetTangentsSmooth();
                    if (enableUndo)
                        Presenter.Undo.AddAction(new EditSplineAction(spline, before));
                    SplineNode.OnSplineEdited(spline);
                    Editor.Instance.Scene.MarkSceneEdited(spline.Scene);
                }
            }
        }
    }
}
