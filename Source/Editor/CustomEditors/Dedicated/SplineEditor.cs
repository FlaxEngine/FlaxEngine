// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEditor.Actions;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEditor.CustomEditors.Elements;
using System;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="Spline"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(Spline)), DefaultEditor]
    public class SplineEditor : ActorEditor
    {
        /// <summary>
        /// Basis for creating tangent manipulation types for bezier curves.
        /// </summary>
        private abstract class TangentModeBase
        {
            /// <summary>
            /// Called when user set selected tangent mode.
            /// </summary>
            /// <param name="spline">Current spline selected on editor viewport.</param>
            /// <param name="index">Index of current keyframe selected on spline.</param>
            public abstract void OnSetMode(Spline spline, int index);

            /// <summary>
            /// Called when user select a keyframe (spline point) of current selected spline on editor viewport.
            /// </summary>
            /// <param name="spline">Current spline selected on editor viewport.</param>
            /// <param name="index">Index of current keyframe selected on spline.</param>
            public abstract void OnSelectKeyframe(Spline spline, int index);

            /// <summary>
            /// Called when user select a tangent of current keyframe selected from spline.
            /// </summary>
            /// <param name="spline">Current spline selected on editor viewport.</param>
            /// <param name="index">Index of current keyframe selected on spline.</param>
            public abstract void OnSelectTangent(Spline spline, int index);

            /// <summary>
            /// Called when the tangent in from current keyframe selected from spline is moved on editor viewport.
            /// </summary>
            /// <param name="spline">Current spline selected on editor viewport.</param>
            /// <param name="index">Index of current keyframe selected on spline.</param>
            public abstract void OnMoveTangentIn(Spline spline, int index);

            /// <summary>
            /// Called when the tangent out from current keyframe selected from spline is moved on editor viewport.
            /// </summary>
            /// <param name="spline">Current spline selected on editor viewport.</param>
            /// <param name="index">Current spline selected on editor viewport.</param>
            public abstract void OnMoveTangentOut(Spline spline, int index);
        }

        /// <summary>
        /// Edit curve options manipulate the curve as free mode
        /// </summary>
        private sealed class FreeTangentMode : TangentModeBase
        {
            /// <inheritdoc/>
            public override void OnMoveTangentIn(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnMoveTangentOut(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectKeyframe(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectTangent(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index) { }
        }

        /// <summary>
        /// Edit curve options to set tangents to linear
        /// </summary>
        private sealed class LinearTangentMode : TangentModeBase
        {
            /// <inheritdoc/>
            public override void OnMoveTangentIn(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnMoveTangentOut(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectKeyframe(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectTangent(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                SetKeyframeLinear(spline, index);
            }

            private void SetKeyframeLinear(Spline spline, int index)
            {
                var tangentIn = spline.GetSplineTangent(index, true);
                var tangentOut = spline.GetSplineTangent(index, false);
                tangentIn.Translation = spline.GetSplinePoint(index);
                tangentOut.Translation = spline.GetSplinePoint(index);
                spline.SetSplineTangent(index, tangentIn, true, false);
                spline.SetSplineTangent(index, tangentOut, false, false);
                spline.UpdateSpline();
            }
        }

        /// <summary>
        /// Edit curve options to align tangents of selected spline
        /// </summary>
        private sealed class AlignedTangentMode : TangentModeBase
        {
            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                SmoothIfNotAligned(spline, index);
            }

            /// <inheritdoc/>
            public override void OnSelectKeyframe(Spline spline, int index)
            {
                SmoothIfNotAligned(spline, index);
            }

            /// <inheritdoc/>
            public override void OnSelectTangent(Spline selectedSpline, int index) { }

            /// <inheritdoc/>
            public override void OnMoveTangentIn(Spline spline, int index)
            {
                SetPointAligned(spline, index, true);
            }

            /// <inheritdoc/>
            public override void OnMoveTangentOut(Spline spline, int index)
            {
                SetPointAligned(spline, index, false);
            }

            private void SmoothIfNotAligned(Spline spline, int index)
            {
                var keyframe = spline.GetSplineKeyframe(index);
                var isAligned = Vector3.Dot(keyframe.TangentIn.Translation.Normalized, keyframe.TangentOut.Translation.Normalized) == 1f;

                if (!isAligned)
                {
                    SetPointSmooth(spline, index);
                }
            }

            private void SetPointSmooth(Spline spline, int index)
            {
                var keyframe = spline.GetSplineKeyframe(index);
                var tangentIn = keyframe.TangentIn;
                var tangentOut = keyframe.TangentOut;
                var tangentInSize = tangentIn.Translation.Length;
                var tangentOutSize = tangentOut.Translation.Length;

                var isLastKeyframe = index >= spline.SplinePointsCount - 1;
                var isFirstKeyframe = index <= 0;

                // force smooth it's linear point
                if (tangentInSize == 0f && !isFirstKeyframe) tangentInSize = 100;
                if (tangentOutSize == 0f && !isLastKeyframe) tangentOutSize = 100;

                var nextKeyframe = !isLastKeyframe ? spline.GetSplineKeyframe(index + 1) : keyframe;
                var previousKeyframe = !isFirstKeyframe ? spline.GetSplineKeyframe(index - 1) : keyframe;

                // calc form from Spline.cpp -> SetTangentsSmooth
                var slop = (keyframe.Value.Translation - previousKeyframe.Value.Translation + nextKeyframe.Value.Translation - keyframe.Value.Translation).Normalized;

                keyframe.TangentIn.Translation = -slop * tangentInSize;
                keyframe.TangentOut.Translation = slop * tangentOutSize;
                spline.SetSplineKeyframe(index, keyframe);
            }

            private void SetPointAligned(Spline spline, int index, bool isIn)
            {
                var keyframe = spline.GetSplineKeyframe(index);
                var referenceTangent = isIn ? keyframe.TangentIn : keyframe.TangentOut;
                var otherTangent = !isIn ? keyframe.TangentIn : keyframe.TangentOut;

                // inverse of reference tangent
                otherTangent.Translation = -referenceTangent.Translation.Normalized * otherTangent.Translation.Length;

                if (isIn) keyframe.TangentOut = otherTangent;
                if (!isIn) keyframe.TangentIn = otherTangent;

                spline.SetSplineKeyframe(index, keyframe);
            }
        }

        private TangentModeBase _currentTangentMode;

        private ButtonElement _freeTangentButton;
        private ButtonElement _linearTangentButton;
        private ButtonElement _alignedTangentButton;

        private bool _tanInChanged;
        private bool _tanOutChanged;
        private Vector3 _lastTanInPos;
        private Vector3 _lastTanOutPos;
        private SplineNode.SplinePointNode _lastPointSelected;
        private SplineNode.SplinePointTangentNode _selectedTangentIn;
        private SplineNode.SplinePointTangentNode _selectedTangentOut;

        /// <summary>
        /// Current selected spline on editor, if has
        /// </summary>
        public Spline SelectedSpline => !Values.HasDifferentValues && Values[0] is Spline ? (Spline)Values[0] : null;

        /// <summary>
        /// Create a Spline editor
        /// </summary>
        public SplineEditor()
        {
            _currentTangentMode = new FreeTangentMode();
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (Values.HasDifferentTypes == false)
            {
                layout.Space(10);

                layout.Header("Selected spline point");
                var selectedPointsGrid = layout.CustomContainer<UniformGridPanel>();
                selectedPointsGrid.CustomControl.SlotsHorizontally = 3;
                selectedPointsGrid.CustomControl.SlotsVertically = 1;

                _linearTangentButton = selectedPointsGrid.Button("Linear");
                _freeTangentButton = selectedPointsGrid.Button("Free");
                _alignedTangentButton = selectedPointsGrid.Button("Aligned");

                _linearTangentButton.Button.Clicked += SetModeLinear;
                _freeTangentButton.Button.Clicked += SetModeFree;
                _alignedTangentButton.Button.Clicked += SetModeAligned;

                layout.Header("All spline points");
                var grid = layout.CustomContainer<UniformGridPanel>();
                grid.CustomControl.SlotsHorizontally = 2;
                grid.CustomControl.SlotsVertically = 1;
                grid.Button("Set Linear Tangents").Button.Clicked += OnSetTangentsLinear;
                grid.Button("Set Smooth Tangents").Button.Clicked += OnSetTangentsSmooth;
            }
        }

        /// <inheritdoc/>
        public override void Refresh()
        {
            base.Refresh();

            UpdateSelectedPoint();
            UpdateSelectedTangent();

            var index = _lastPointSelected.Index;
            var currentTangentInPosition = SelectedSpline.GetSplineLocalTangent(index, true).Translation;
            var currentTangentOutPosition = SelectedSpline.GetSplineLocalTangent(index, false).Translation;

            if (_selectedTangentIn != null)
            {
                _tanInChanged = _lastTanInPos != currentTangentInPosition;
                _lastTanInPos = currentTangentInPosition;
            }

            if (_selectedTangentOut != null)
            {
                _tanOutChanged = _lastTanOutPos != currentTangentOutPosition;
                _lastTanOutPos = currentTangentOutPosition;
            }

            if (_tanInChanged) _currentTangentMode.OnMoveTangentIn(SelectedSpline, index);
            if (_tanOutChanged) _currentTangentMode.OnMoveTangentOut(SelectedSpline, index);

            currentTangentInPosition = SelectedSpline.GetSplineLocalTangent(index, true).Translation;
            currentTangentOutPosition = SelectedSpline.GetSplineLocalTangent(index, false).Translation;

            // update last tangents position after changes
            if (SelectedSpline) _lastTanInPos = currentTangentInPosition;
            if (SelectedSpline) _lastTanOutPos = currentTangentOutPosition;
            _tanInChanged = false;
            _tanOutChanged = false;
        }

        private void SetModeLinear()
        {
            _currentTangentMode = new LinearTangentMode();
            _currentTangentMode.OnSetMode(SelectedSpline, _lastPointSelected.Index);
        }

        private void SetModeFree()
        {
            _currentTangentMode = new FreeTangentMode();
            _currentTangentMode.OnSetMode(SelectedSpline, _lastPointSelected.Index);
        }

        private void SetModeAligned()
        {
            _currentTangentMode = new AlignedTangentMode();
            _currentTangentMode.OnSetMode(SelectedSpline, _lastPointSelected.Index);
        }

        private void UpdateSelectedPoint()
        {
            // works only if select one spline
            if (Editor.Instance.SceneEditing.SelectionCount != 1) return;

            var currentSelected = Editor.Instance.SceneEditing.Selection[0];

            if (currentSelected == _lastPointSelected) return;
            if (currentSelected is not SplineNode.SplinePointNode) return;

            _lastPointSelected = currentSelected as SplineNode.SplinePointNode;
            var index = _lastPointSelected.Index;
            _currentTangentMode.OnSelectKeyframe(SelectedSpline, index);
        }

        private void UpdateSelectedTangent()
        {
            // works only if select one spline
            if (_lastPointSelected == null || Editor.Instance.SceneEditing.SelectionCount != 1) return;

            var currentSelected = Editor.Instance.SceneEditing.Selection[0];

            if (currentSelected is not SplineNode.SplinePointTangentNode) return;
            if (currentSelected == _selectedTangentIn) return;
            if (currentSelected == _selectedTangentOut) return;

            var index = _lastPointSelected.Index;

            if (currentSelected.Transform == SelectedSpline.GetSplineTangent(index, true))
            {
                _selectedTangentIn = currentSelected as SplineNode.SplinePointTangentNode;
                _currentTangentMode.OnSelectTangent(SelectedSpline, index);
                return;
            }

            if (currentSelected.Transform == SelectedSpline.GetSplineTangent(index, false))
            {
                _selectedTangentOut = currentSelected as SplineNode.SplinePointTangentNode;
                _currentTangentMode.OnSelectTangent(SelectedSpline, index);
                return;
            }

            _selectedTangentIn = null;
            _selectedTangentOut = null;
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
