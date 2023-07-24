// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
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
        /// Storage undo spline data
        /// </summary>
        private struct UndoData
        {
            public Spline spline;
            public BezierCurve<Transform>.Keyframe[] beforeKeyframes;
        }

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

            private void SetPointAligned(Spline spline, int index, bool alignWithIn)
            {
                var keyframe = spline.GetSplineKeyframe(index);
                var referenceTangent = alignWithIn ? keyframe.TangentIn : keyframe.TangentOut;
                var otherTangent = !alignWithIn ? keyframe.TangentIn : keyframe.TangentOut;

                // inverse of reference tangent
                otherTangent.Translation = -referenceTangent.Translation.Normalized * otherTangent.Translation.Length;

                if (alignWithIn) keyframe.TangentOut = otherTangent;
                if (!alignWithIn) keyframe.TangentIn = otherTangent;

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
                SetTangentSmoothIn(spline, index);
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
                SetTangentSmoothOut(spline, index);
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

        private readonly Color HighlightedColor = FlaxEngine.GUI.Style.Current.BackgroundSelected;
        private readonly Color SelectedButtonColor = FlaxEngine.GUI.Style.Current.BackgroundSelected;
        private readonly Color NormalButtonColor = FlaxEngine.GUI.Style.Current.BackgroundNormal;

        private EditTangentOptionBase _currentTangentMode;

        private ButtonElement _freeTangentButton;
        private ButtonElement _linearTangentButton;
        private ButtonElement _alignedTangentButton;
        private ButtonElement _smoothInTangentButton;
        private ButtonElement _smoothOutTangentButton;
        private ButtonElement _setLinearAllTangentsButton;
        private ButtonElement _setSmoothAllTangentsButton;

        private bool _tanInChanged;
        private bool _tanOutChanged;
        private Vector3 _lastTanInPos;
        private Vector3 _lastTanOutPos;
        private Spline _selectedSpline;
        private SplineNode.SplinePointNode _selectedPoint;
        private SplineNode.SplinePointNode _lastPointSelected;
        private SplineNode.SplinePointTangentNode _selectedTangentIn;
        private SplineNode.SplinePointTangentNode _selectedTangentOut;

        private UndoData[] selectedSplinesUndoData;

        private bool HasPointSelected => _selectedPoint != null;
        private bool HasTangentsSelected => _selectedTangentIn != null || _selectedTangentOut != null;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            _currentTangentMode = new FreeTangentMode();

            if (Values.HasDifferentTypes == false)
            {
                _selectedSpline = !Values.HasDifferentValues && Values[0] is Spline ? (Spline)Values[0] : null;

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

                _linearTangentButton.Button.BackgroundColorHighlighted = HighlightedColor;
                _freeTangentButton.Button.BackgroundColorHighlighted = HighlightedColor;
                _alignedTangentButton.Button.BackgroundColorHighlighted = HighlightedColor;
                _smoothInTangentButton.Button.BackgroundColorHighlighted = HighlightedColor;
                _smoothOutTangentButton.Button.BackgroundColorHighlighted = HighlightedColor;

                _linearTangentButton.Button.Clicked += StartEditSpline;
                _freeTangentButton.Button.Clicked += StartEditSpline;
                _alignedTangentButton.Button.Clicked += StartEditSpline;
                _smoothInTangentButton.Button.Clicked += StartEditSpline;
                _smoothOutTangentButton.Button.Clicked += StartEditSpline;

                _linearTangentButton.Button.Clicked += SetModeLinear;
                _freeTangentButton.Button.Clicked += SetModeFree;
                _alignedTangentButton.Button.Clicked += SetModeAligned;
                _smoothInTangentButton.Button.Clicked += SetModeSmoothIn;
                _smoothOutTangentButton.Button.Clicked += SetModeSmoothOut;

                _linearTangentButton.Button.Clicked += EndEditSpline;
                _freeTangentButton.Button.Clicked += EndEditSpline;
                _alignedTangentButton.Button.Clicked += EndEditSpline;
                _smoothInTangentButton.Button.Clicked += EndEditSpline;
                _smoothOutTangentButton.Button.Clicked += EndEditSpline;

                layout.Header("All spline points");
                var grid = layout.CustomContainer<UniformGridPanel>();
                grid.CustomControl.SlotsHorizontally = 2;
                grid.CustomControl.SlotsVertically = 1;

                _setLinearAllTangentsButton = grid.Button("Set Linear Tangents");
                _setSmoothAllTangentsButton = grid.Button("Set Smooth Tangents");
                _setLinearAllTangentsButton.Button.BackgroundColorHighlighted = HighlightedColor;
                _setSmoothAllTangentsButton.Button.BackgroundColorHighlighted = HighlightedColor;

                _setLinearAllTangentsButton.Button.Clicked += StartEditSpline;
                _setSmoothAllTangentsButton.Button.Clicked += StartEditSpline;

                _setLinearAllTangentsButton.Button.Clicked += OnSetTangentsLinear;
                _setSmoothAllTangentsButton.Button.Clicked += OnSetTangentsSmooth;

                _setLinearAllTangentsButton.Button.Clicked += EndEditSpline;
                _setSmoothAllTangentsButton.Button.Clicked += EndEditSpline;

                if (_selectedSpline) _selectedSpline.SplineUpdated += OnSplineEdited;
            }
        }

        /// <inheritdoc/>
        protected override void Deinitialize()
        {
            if (_selectedSpline) _selectedSpline.SplineUpdated -= OnSplineEdited;
        }

        private void OnSplineEdited()
        {
            SetSelectedTangentTypeAsCurrent();
            UpdateButtonsColors();
        }

        /// <inheritdoc/>
        public override void Refresh()
        {
            base.Refresh();

            UpdateSelectedPoint();
            UpdateSelectedTangent();

            _linearTangentButton.Button.Enabled = CanEditTangent();
            _freeTangentButton.Button.Enabled = CanSetTangentFree();
            _alignedTangentButton.Button.Enabled = CanSetTangentAligned();
            _smoothInTangentButton.Button.Enabled = CanSetTangentSmoothIn();
            _smoothOutTangentButton.Button.Enabled = CanSetTangentSmoothOut();
            _setLinearAllTangentsButton.Button.Enabled = CanEditTangent();
            _setSmoothAllTangentsButton.Button.Enabled = CanEditTangent();

            if (!CanEditTangent())
            {
                return;
            }

            var index = _lastPointSelected.Index;
            var currentTangentInPosition = _selectedSpline.GetSplineLocalTangent(index, true).Translation;
            var currentTangentOutPosition = _selectedSpline.GetSplineLocalTangent(index, false).Translation;

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

            if (_tanInChanged) _currentTangentMode.OnMoveTangentIn(_selectedSpline, index);
            if (_tanOutChanged) _currentTangentMode.OnMoveTangentOut(_selectedSpline, index);

            currentTangentInPosition = _selectedSpline.GetSplineLocalTangent(index, true).Translation;
            currentTangentOutPosition = _selectedSpline.GetSplineLocalTangent(index, false).Translation;

            // update last tangents position after changes
            if (_selectedSpline) _lastTanInPos = currentTangentInPosition;
            if (_selectedSpline) _lastTanOutPos = currentTangentOutPosition;
            _tanInChanged = false;
            _tanOutChanged = false;
        }

        private bool CanEditTangent()
        {
            return !HasDifferentTypes && !HasDifferentValues && (HasPointSelected || HasTangentsSelected);
        }

        private bool CanSetTangentSmoothIn()
        {
            if (!CanEditTangent()) return false;
            return _lastPointSelected.Index != 0;
        }

        private bool CanSetTangentSmoothOut()
        {
            if (!CanEditTangent()) return false;
            return _lastPointSelected.Index < _selectedSpline.SplinePointsCount - 1;
        }

        private bool CanSetTangentFree()
        {
            if (!CanEditTangent()) return false;
            return _lastPointSelected.Index < _selectedSpline.SplinePointsCount - 1 && _lastPointSelected.Index != 0;
        }

        private bool CanSetTangentAligned()
        {
            if (!CanEditTangent()) return false;
            return _lastPointSelected.Index < _selectedSpline.SplinePointsCount - 1 && _lastPointSelected.Index != 0; 
        }

        private void SetModeLinear()
        {
            if (_currentTangentMode is LinearTangentMode) return;
            _currentTangentMode = new LinearTangentMode();
            _currentTangentMode.OnSetMode(_selectedSpline, _lastPointSelected.Index);
        }

        private void SetModeFree()
        {
            if (_currentTangentMode is FreeTangentMode) return;
            _currentTangentMode = new FreeTangentMode();
            _currentTangentMode.OnSetMode(_selectedSpline, _lastPointSelected.Index);
        }

        private void SetModeAligned()
        {
            if (_currentTangentMode is AlignedTangentMode) return;
            _currentTangentMode = new AlignedTangentMode();
            _currentTangentMode.OnSetMode(_selectedSpline, _lastPointSelected.Index);
        }

        private void SetModeSmoothIn()
        {
            if (_currentTangentMode is SmoothInTangentMode) return;
            _currentTangentMode = new SmoothInTangentMode();
            _currentTangentMode.OnSetMode(_selectedSpline, _lastPointSelected.Index);
        }

        private void SetModeSmoothOut()
        {
            if (_currentTangentMode is SmoothOutTangentMode) return;
            _currentTangentMode = new SmoothOutTangentMode();
            _currentTangentMode.OnSetMode(_selectedSpline, _lastPointSelected.Index);
        }

        private void UpdateSelectedPoint()
        {
            // works only if select one spline
            if (Editor.Instance.SceneEditing.SelectionCount != 1)
            {
                _selectedPoint = null;
                UpdateButtonsColors();
                return;
            }

            var currentSelected = Editor.Instance.SceneEditing.Selection[0];

            if (currentSelected == _selectedPoint) return;
            if (currentSelected is SplineNode.SplinePointNode)
            {
                _selectedPoint = currentSelected as SplineNode.SplinePointNode;
                _lastPointSelected = _selectedPoint;

                var index = _lastPointSelected.Index;

                SetSelectedTangentTypeAsCurrent();
                UpdateButtonsColors();

                _currentTangentMode.OnSelectKeyframe(_selectedSpline, index);
            }
            else
            {
                _selectedPoint = null;
                UpdateButtonsColors();
            }
        }

        private void UpdateSelectedTangent()
        {
            // works only if select one spline
            if (_lastPointSelected == null || Editor.Instance.SceneEditing.SelectionCount != 1)
            {
                _selectedTangentIn = null;
                _selectedTangentOut = null;
                return;
            }

            var currentSelected = Editor.Instance.SceneEditing.Selection[0];

            if (currentSelected is not SplineNode.SplinePointTangentNode)
            {
                _selectedTangentIn = null;
                _selectedTangentOut = null;
                return;
            }

            if (currentSelected == _selectedTangentIn) return;
            if (currentSelected == _selectedTangentOut) return;

            var index = _lastPointSelected.Index;

            if (currentSelected.Transform == _selectedSpline.GetSplineTangent(index, true))
            {
                _selectedTangentIn = currentSelected as SplineNode.SplinePointTangentNode;
                _selectedTangentOut = null;
                _currentTangentMode.OnSelectTangent(_selectedSpline, index);
                
                return;
            }

            if (currentSelected.Transform == _selectedSpline.GetSplineTangent(index, false))
            {
                _selectedTangentOut = currentSelected as SplineNode.SplinePointTangentNode;
                _selectedTangentIn = null;
                _currentTangentMode.OnSelectTangent(_selectedSpline, index);
                return;
            }

            _selectedTangentIn = null;
            _selectedTangentOut = null;
        }

        private void UpdateButtonsColors()
        {
            if (!CanEditTangent())
            {
                _linearTangentButton.Button.BackgroundColor = NormalButtonColor;
                _freeTangentButton.Button.BackgroundColor = NormalButtonColor;
                _alignedTangentButton.Button.BackgroundColor = NormalButtonColor;
                _smoothInTangentButton.Button.BackgroundColor = NormalButtonColor;
                _smoothOutTangentButton.Button.BackgroundColor = NormalButtonColor;
                return;
            }

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
            if (_lastPointSelected == null || _selectedPoint == null) return;
            if (IsLinearTangentMode(_selectedSpline, _lastPointSelected.Index)) SetModeLinear();
            else if (IsAlignedTangentMode(_selectedSpline, _lastPointSelected.Index)) SetModeAligned();
            else if (IsSmoothInTangentMode(_selectedSpline, _lastPointSelected.Index)) SetModeSmoothIn();
            else if (IsSmoothOutTangentMode(_selectedSpline, _lastPointSelected.Index)) SetModeSmoothOut();
            else if (IsFreeTangentMode(_selectedSpline, _lastPointSelected.Index)) SetModeFree();
        }

        private void StartEditSpline()
        {
            var enableUndo = Presenter.Undo != null && Presenter.Undo.Enabled;

            if (!enableUndo)
            {
                return;
            }

            var splines = new List<UndoData>();

            for (int i = 0; i < Values.Count; i++)
            {
                if (Values[i] is Spline spline)
                {
                    splines.Add(new UndoData { 
                        spline = spline,
                        beforeKeyframes = spline.SplineKeyframes.Clone() as BezierCurve<Transform>.Keyframe[]
                    });
                }
            }

            selectedSplinesUndoData = splines.ToArray();
        }

        private void EndEditSpline()
        {
            var enableUndo = Presenter.Undo != null && Presenter.Undo.Enabled;

            if (!enableUndo)
            {
                return;
            }

            for (int i = 0; i < selectedSplinesUndoData.Length; i++)
            {
                var splineUndoData = selectedSplinesUndoData[i];
                Presenter.Undo.AddAction(new EditSplineAction(_selectedSpline, splineUndoData.beforeKeyframes));
                SplineNode.OnSplineEdited(splineUndoData.spline);
                Editor.Instance.Scene.MarkSceneEdited(splineUndoData.spline.Scene);
            }
        }

        private void OnSetTangentsLinear()
        {
            _selectedSpline.SetTangentsLinear();
            _selectedSpline.UpdateSpline();
        }

        private void OnSetTangentsSmooth()
        {
            _selectedSpline.SetTangentsSmooth();
            _selectedSpline.UpdateSpline();
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

        private static void SetKeyframeLinear(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            keyframe.TangentIn.Translation = Vector3.Zero;
            keyframe.TangentOut.Translation = Vector3.Zero;

            var lastSplineIndex = spline.SplinePointsCount - 1;
            if (index == lastSplineIndex && spline.IsLoop)
            {
                var lastPoint = spline.GetSplineKeyframe(lastSplineIndex);
                lastPoint.TangentIn.Translation = Vector3.Zero;
                lastPoint.TangentOut.Translation = Vector3.Zero;
                spline.SetSplineKeyframe(lastSplineIndex, lastPoint);
            }

            spline.SetSplineKeyframe(index, keyframe);
            spline.UpdateSpline();
        }

        private static void SetTangentSmoothIn(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);

            // auto smooth tangent if's linear
            if (keyframe.TangentIn.Translation.Length == 0)
            {
                var previousKeyframe = spline.GetSplineKeyframe(index - 1);
                var tangentDirection = keyframe.Value.WorldToLocalVector(previousKeyframe.Value.Translation - keyframe.Value.Translation);
                tangentDirection = tangentDirection.Normalized * 100f;
                keyframe.TangentIn.Translation = tangentDirection;
            }

            keyframe.TangentOut.Translation = Vector3.Zero;
            spline.SetSplineKeyframe(index, keyframe);
            spline.UpdateSpline();
        }

        private static void SetTangentSmoothOut(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);

            // auto smooth tangent if's linear
            if (keyframe.TangentOut.Translation.Length == 0)
            {
                var nextKeyframe = spline.GetSplineKeyframe(index + 1);
                var tangentDirection = keyframe.Value.WorldToLocalVector(nextKeyframe.Value.Translation - keyframe.Value.Translation);
                tangentDirection = tangentDirection.Normalized * 100f;
                keyframe.TangentOut.Translation = tangentDirection;
            }

            keyframe.TangentIn.Translation = Vector3.Zero;

            spline.SetSplineKeyframe(index, keyframe);
            spline.UpdateSpline();
        }

        private static void SetPointSmooth(Spline spline, int index)
        {
            var keyframe = spline.GetSplineKeyframe(index);
            var tangentInSize = keyframe.TangentIn.Translation.Length;
            var tangentOutSize = keyframe.TangentOut.Translation.Length;

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
            spline.UpdateSpline();
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
