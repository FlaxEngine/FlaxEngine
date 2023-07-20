// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEditor.Actions;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.CustomEditors.Elements;

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
        private abstract class EditTangentOptionBase
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
        private sealed class FreeTangentMode : EditTangentOptionBase
        {
            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                if (IsLinearTangentMode(spline, index))
                {
                    SetPointSmooth(spline, index);
                }
            }

            /// <inheritdoc/>
            public override void OnMoveTangentIn(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnMoveTangentOut(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectKeyframe(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectTangent(Spline spline, int index) { }
        }

        /// <summary>
        /// Edit curve options to set tangents to linear
        /// </summary>
        private sealed class LinearTangentMode : EditTangentOptionBase
        {
            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                SetKeyframeLinear(spline, index);

                // change the selection to tangent parent (a spline point / keyframe)
                SetSelectSplinePointNode(spline, index);
            }

            /// <inheritdoc/>
            public override void OnMoveTangentIn(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnMoveTangentOut(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectKeyframe(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectTangent(Spline spline, int index) { }

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
        private sealed class AlignedTangentMode : EditTangentOptionBase
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
                if (!IsAlignedTangentMode(spline, index))
                {
                    SetPointSmooth(spline, index);
                }
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

        /// <summary>
        /// Edit curve options manipulate the curve setting selected point 
        /// tangent in as smoothed but tangent out as linear
        /// </summary>
        private sealed class SmoothInTangentMode : EditTangentOptionBase
        {
            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                var keyframe = spline.GetSplineKeyframe(index);

                // auto smooth tangent if's linear
                if (keyframe.TangentIn.Translation.Length == 0)
                {
                    var isLastKeyframe = index == spline.SplinePointsCount - 1;

                    if (!isLastKeyframe)
                    {
                        var nexKeyframe = spline.GetSplineKeyframe(index + 1);
                        var directionToNextKeyframe = keyframe.Value.WorldToLocalVector(keyframe.Value.Translation - nexKeyframe.Value.Translation);
                        directionToNextKeyframe = directionToNextKeyframe.Normalized * 100f;
                        keyframe.TangentIn.Translation = directionToNextKeyframe;
                    }
                }

                keyframe.TangentOut.Translation = Vector3.Zero;

                spline.SetSplineKeyframe(index, keyframe);
                SetSelectTangentIn(spline, index);
            }

            /// <inheritdoc/>
            public override void OnMoveTangentIn(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnMoveTangentOut(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectKeyframe(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectTangent(Spline spline, int index) { }
        }

        /// <summary>
        /// Edit curve options manipulate the curve setting selected point 
        /// tangent in as linear but tangent out as smoothed
        /// </summary>
        private sealed class SmoothOutTangentMode : EditTangentOptionBase
        {
            /// <inheritdoc/>
            public override void OnSetMode(Spline spline, int index)
            {
                var keyframe = spline.GetSplineKeyframe(index);

                // auto smooth tangent if's linear
                if (keyframe.TangentOut.Translation.Length == 0)
                {
                    var isFirstKeyframe = index == 0;

                    if (!isFirstKeyframe)
                    {
                        var previousKeyframe = spline.GetSplineKeyframe(index - 1);
                        var directionToPreviousKeyframe = keyframe.Value.WorldToLocalVector(keyframe.Value.Translation - previousKeyframe.Value.Translation);
                        directionToPreviousKeyframe = directionToPreviousKeyframe.Normalized * 100f;
                        keyframe.TangentOut.Translation = directionToPreviousKeyframe;
                    }
                }

                keyframe.TangentIn.Translation = Vector3.Zero;

                spline.SetSplineKeyframe(index, keyframe);
                SetSelectTangentOut(spline, index);
            }

            /// <inheritdoc/>
            public override void OnMoveTangentIn(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnMoveTangentOut(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectKeyframe(Spline spline, int index) { }

            /// <inheritdoc/>
            public override void OnSelectTangent(Spline spline, int index) { }
        }

        private EditTangentOptionBase _currentTangentMode;

        private ButtonElement _freeTangentButton;
        private ButtonElement _linearTangentButton;
        private ButtonElement _alignedTangentButton;
        private ButtonElement _smoothInTangentButton;
        private ButtonElement _smoothOutTangentButton;

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

        private bool HasTangentsSelected => _selectedTangentIn != null || _selectedTangentOut != null;

        private bool HasPointSelected => _lastPointSelected != null;

        private bool CanSetTangentMode => HasPointSelected || HasTangentsSelected;

        private Color SelectedButtonColor => FlaxEngine.GUI.Style.Current.BackgroundSelected;

        private Color NormalButtonColor => FlaxEngine.GUI.Style.Current.BackgroundNormal;

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
                selectedPointsGrid.CustomControl.SlotsVertically = 2;

                selectedPointsGrid.Control.Size *= new Float2(1, 2);

                _linearTangentButton = selectedPointsGrid.Button("Linear");
                _freeTangentButton = selectedPointsGrid.Button("Free");
                _alignedTangentButton = selectedPointsGrid.Button("Aligned");
                _smoothInTangentButton = selectedPointsGrid.Button("Smooth In");
                _smoothOutTangentButton = selectedPointsGrid.Button("Smooth Out");

                _linearTangentButton.Button.Clicked += SetModeLinear;
                _freeTangentButton.Button.Clicked += SetModeFree;
                _alignedTangentButton.Button.Clicked += SetModeAligned;
                _smoothInTangentButton.Button.Clicked += SetModeSmoothIn;
                _smoothOutTangentButton.Button.Clicked += SetModeSmoothOut;

                _linearTangentButton.Button.Clicked += UpdateButtonsColors;
                _freeTangentButton.Button.Clicked += UpdateButtonsColors;
                _alignedTangentButton.Button.Clicked += UpdateButtonsColors;
                _smoothInTangentButton.Button.Clicked += UpdateButtonsColors;
                _smoothOutTangentButton.Button.Clicked += UpdateButtonsColors;

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

            _freeTangentButton.Button.Enabled = CanSetTangentMode;
            _linearTangentButton.Button.Enabled = CanSetTangentMode;
            _alignedTangentButton.Button.Enabled = CanSetTangentMode;
            _smoothInTangentButton.Button.Enabled = CanSetTangentMode;
            _smoothOutTangentButton.Button.Enabled = CanSetTangentMode;

            if (_lastPointSelected == null)
            {
                return;
            }

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

        private void SetModeSmoothIn()
        {
            _currentTangentMode = new SmoothInTangentMode();
            _currentTangentMode.OnSetMode(SelectedSpline, _lastPointSelected.Index);
        }

        private void SetModeSmoothOut()
        {
            _currentTangentMode = new SmoothOutTangentMode();
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

            SetSelectedTangentTypeAsCurrent();
            UpdateButtonsColors();

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

        private void UpdateButtonsColors()
        {
            var isFree = _currentTangentMode is FreeTangentMode;
            var isLinear = _currentTangentMode is LinearTangentMode;
            var isAligned = _currentTangentMode is AlignedTangentMode;
            var isSmoothIn = _currentTangentMode is SmoothInTangentMode;
            var isSmoothOut = _currentTangentMode is SmoothOutTangentMode;

            _linearTangentButton.Button.BackgroundColor = isLinear ? SelectedButtonColor : NormalButtonColor;
            _freeTangentButton.Button.BackgroundColor = isFree ? SelectedButtonColor : NormalButtonColor;
            _alignedTangentButton.Button.BackgroundColor = isAligned ? SelectedButtonColor : NormalButtonColor;
            _smoothInTangentButton.Button.BackgroundColor = isSmoothIn ? SelectedButtonColor : NormalButtonColor;
            _smoothOutTangentButton.Button.BackgroundColor = isSmoothOut ? SelectedButtonColor : NormalButtonColor;
        }

        private void SetSelectedTangentTypeAsCurrent()
        {
            var isFree = IsFreeTangentMode(SelectedSpline, _lastPointSelected.Index);
            var isLinear = IsLinearTangentMode(SelectedSpline, _lastPointSelected.Index);
            var isAligned = IsAlignedTangentMode(SelectedSpline, _lastPointSelected.Index);
            var isSmoothIn = IsSmoothInTangentMode(SelectedSpline, _lastPointSelected.Index);
            var isSmoothOut = IsSmoothOutTangentMode(SelectedSpline, _lastPointSelected.Index);

            if (isFree) SetModeFree();
            else if (isLinear) SetModeLinear();
            else if (isAligned) SetModeAligned();
            else if (isSmoothIn) SetModeSmoothIn();
            else if (isSmoothOut) SetModeSmoothIn();
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

        private static bool IsFreeTangentMode(Spline spline, int index)
        {
            if (IsLinearTangentMode(spline, index) || 
                IsAlignedTangentMode(spline, index) ||
                IsSmoothInTangentMode(spline, index) ||
                IsSmoothOutTangentMode(spline, index))
            {
                return false;
            }

            return true;
        }

        private static bool IsLinearTangentMode(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            return keyframe.TangentIn.Translation.Length == 0 && keyframe.TangentOut.Translation.Length == 0;
        }

        private static bool IsAlignedTangentMode(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            var tangentIn = keyframe.TangentIn.Translation;
            var tangentOut = keyframe.TangentOut.Translation;

            if (tangentIn.Length == 0 || tangentOut.Length == 0)
            {
                return false;
            }

            var angleBetweenTwoTangents = Vector3.Dot(tangentIn.Normalized, tangentOut.Normalized);

            if (angleBetweenTwoTangents < -0.99f)
            {
                return true;
            }

            return false;
        }

        private static bool IsSmoothInTangentMode(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            return keyframe.TangentIn.Translation.Length > 0 && keyframe.TangentOut.Translation.Length == 0;
        }

        private static bool IsSmoothOutTangentMode(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            return keyframe.TangentOut.Translation.Length > 0 && keyframe.TangentIn.Translation.Length == 0;
        }

        private static void SetPointSmooth(Spline spline, int index)
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

        private static SplineNode.SplinePointNode GetSplinePointNode(Spline spline, int index)
        {
            return (SplineNode.SplinePointNode)SceneGraphFactory.FindNode(spline.ID).ChildNodes[index];
        }

        private static SplineNode.SplinePointTangentNode GetSplineTangentInNode(Spline spline, int index)
        {
            var point = GetSplinePointNode(spline, index);
            var tangentIn = spline.GetSplineTangent(index, true);
            var tangentNodes = point.ChildNodes;

            // find tangent in node comparing all child nodes position
            for (int i = 0; i < tangentNodes.Count; i++)
            {
                if (tangentNodes[i].Transform.Translation == tangentIn.Translation)
                {
                    return (SplineNode.SplinePointTangentNode)tangentNodes[i];
                }
            }

            return null;
        }

        private static SplineNode.SplinePointTangentNode GetSplineTangentOutNode(Spline spline, int index)
        {
            var point = GetSplinePointNode(spline, index);
            var tangentOut = spline.GetSplineTangent(index, false);
            var tangentNodes = point.ChildNodes;

            // find tangent out node comparing all child nodes position
            for (int i = 0; i < tangentNodes.Count; i++)
            {
                if (tangentNodes[i].Transform.Translation == tangentOut.Translation)
                {
                    return (SplineNode.SplinePointTangentNode)tangentNodes[i];
                }
            }

            return null;
        }

        private static void SetSelectSplinePointNode(Spline spline, int index)
        {
            Editor.Instance.SceneEditing.Select(GetSplinePointNode(spline, index));
        }

        private static void SetSelectTangentIn(Spline spline, int index)
        {
            Editor.Instance.SceneEditing.Select(GetSplineTangentInNode(spline, index));
        }

        private static void SetSelectTangentOut(Spline spline, int index)
        {
            Editor.Instance.SceneEditing.Select(GetSplineTangentOutNode(spline, index));
        }
    }
}
