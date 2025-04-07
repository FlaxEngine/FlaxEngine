// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.GUI.Dialogs;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.GUI
{
    /// <summary>
    /// The color gradient editing control for a timeline media event. Allows to edit the gradients stops to create the linear color animation over time.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class GradientEditor : ContainerControl
    {
        /// <summary>
        /// The gradient stop.
        /// </summary>
        public struct Stop
        {
            /// <summary>
            /// The gradient stop frame position (on time axis, relative to the event start).
            /// </summary>
            [EditorOrder(0), Tooltip("The gradient stop frame position (on time axis, relative to the event start).")]
            public int Frame;

            /// <summary>
            /// The color gradient value.
            /// </summary>
            [CustomEditor(typeof(CustomEditors.Editors.GenericEditor))] // Don't use default editor with color picker (focus change issue)
            [EditorOrder(1), Tooltip("The color gradient value.")]
            public Color Value;
        }

        /// <summary>
        /// The stop control type.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Control" />
        public class StopControl : Control
        {
            private bool _isMoving;
            private Float2 _startMovePos;
            private IColorPickerDialog _currentDialog;

            /// <summary>
            /// The gradient editor reference.
            /// </summary>
            public GradientEditor Gradient;

            /// <summary>
            /// The gradient stop index.
            /// </summary>
            public int Index;

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var isMouseOver = IsMouseOver;
                var color = Gradient._data[Index].Value;
                var icons = Editor.Instance.Icons;
                var icon = icons.VisjectBoxClosed32;

                Render2D.DrawSprite(icon, new Rectangle(0.0f, 0.0f, 10.0f, 10.0f), isMouseOver ? Color.Gray : Color.Black);
                Render2D.DrawSprite(icon, new Rectangle(1.0f, 1.0f, 8.0f, 8.0f), color);
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left)
                {
                    Gradient.Select(this);
                    _isMoving = true;
                    _startMovePos = location;
                    Gradient.OnEditingStart();
                    StartMouseCapture();
                    return true;
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _isMoving)
                {
                    _isMoving = false;
                    EndMouseCapture();
                    Gradient.OnEditingEnd();
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
            {
                // Don't show tooltip is user is moving the stop
                return base.OnShowTooltip(out text, out location, out area) && !_isMoving;
            }

            /// <inheritdoc />
            public override bool OnTestTooltipOverControl(ref Float2 location)
            {
                // Don't show tooltip is user is moving the stop
                return base.OnTestTooltipOverControl(ref location) && !_isMoving;
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                if (_isMoving && Float2.DistanceSquared(ref location, ref _startMovePos) > 25.0f)
                {
                    _startMovePos = Float2.Minimum;
                    var x = PointToParent(location).X;
                    var frame = (int)((x - Width * 0.5f) / Gradient._scale);
                    Gradient.SetStopFrame(Index, frame);
                }

                base.OnMouseMove(location);
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                if (base.OnMouseDoubleClick(location, button))
                    return true;

                if (button == MouseButton.Left)
                {
                    // Show color picker dialog
                    _currentDialog = ColorValueBox.ShowPickColorDialog?.Invoke(this, Gradient._data[Index].Value, OnColorChanged, null, false);
                    return true;
                }

                return false;
            }

            private void OnColorChanged(Color color, bool sliding)
            {
                Gradient.OnEditingStart();
                Gradient.SetStopColor(Index, color);
                Gradient.OnEditingEnd();
            }

            /// <inheritdoc />
            public override void OnEndMouseCapture()
            {
                _isMoving = false;

                base.OnEndMouseCapture();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                if (_currentDialog != null)
                {
                    _currentDialog.ClosePicker();
                    _currentDialog = null;
                }

                base.OnDestroy();
            }
        }

        private List<Stop> _data = new List<Stop>();
        private List<StopControl> _stops = new List<StopControl>();
        private StopControl _selected;
        private float _scale;
        private UpdateDelegate _update;

        /// <summary>
        /// Gets or sets the list of gradient stops.
        /// </summary>
        public List<Stop> Stops
        {
            get => _data;
            set
            {
                if (value == null)
                    throw new ArgumentNullException();
                if (value.SequenceEqual(_data))
                    return;

                _data.Clear();
                _data.AddRange(value);
                _data.Sort((a, b) => a.Frame > b.Frame ? 1 : 0);
                OnStopsChanged();
                UpdateControls();
            }
        }

        /// <summary>
        /// Occurs when stops collection gets changed (added/removed).
        /// </summary>
        public event Action StopsChanged;

        /// <summary>
        /// Occurs when stops collection gets modified (stop value or time modified).
        /// </summary>
        public event Action Edited;

        /// <summary>
        /// Occurs when gradient data editing starts (via UI).
        /// </summary>
        public event Action EditingStart;

        /// <summary>
        /// Occurs when gradient data editing ends (via UI).
        /// </summary>
        public event Action EditingEnd;

        /// <summary>
        /// Initializes a new instance of the <see cref="GradientEditor"/> class.
        /// </summary>
        public GradientEditor()
        {
            AutoFocus = false;
            SetUpdate(ref _update, OnUpdate);
        }

        private void OnUpdate(float deltaTime)
        {
            // Required to synchronize controls with Stops array edited via property edit context menu
            _data.Sort((a, b) => a.Frame > b.Frame ? 1 : 0);
            UpdateControls();
        }

        /// <summary>
        /// Sets the scale factor (used to convert the gradient stops frame into control pixels).
        /// </summary>
        /// <param name="scale">The scale.</param>
        public void SetScale(float scale)
        {
            _scale = scale;
            UpdateControls();
        }

        /// <summary>
        /// Called when timeline FPS gets changed.
        /// </summary>
        /// <param name="before">The before value.</param>
        /// <param name="after">The after value.</param>
        public void OnTimelineFpsChanged(float before, float after)
        {
            for (int i = 0; i < _data.Count; i++)
            {
                var stop = _data[i];
                stop.Frame = (int)((stop.Frame / before) * after);
                _data[i] = stop;
            }
            UpdateControls();
        }

        /// <summary>
        /// Called when gradient data editing starts (via UI).
        /// </summary>
        public void OnEditingStart()
        {
            EditingStart?.Invoke();
        }

        /// <summary>
        /// Called when gradient data editing ends (via UI).
        /// </summary>
        public void OnEditingEnd()
        {
            EditingEnd?.Invoke();
        }

        /// <summary>
        /// Called when stops collection gets changed (added/removed).
        /// </summary>
        protected virtual void OnStopsChanged()
        {
            StopsChanged?.Invoke();
        }

        /// <summary>
        /// Called when stops collection gets modified (stop value or time modified).
        /// </summary>
        protected virtual void OnEdited()
        {
            Edited?.Invoke();
        }

        private void Select(StopControl stop)
        {
            _selected = stop;
            UpdateControls();
        }

        private void SetStopFrame(int index, int frame)
        {
            if (index != 0)
            {
                frame = Mathf.Max(frame, _data[index - 1].Frame);
            }
            if (index != _stops.Count - 1)
            {
                frame = Mathf.Min(frame, _data[index + 1].Frame);
            }

            var stop = _data[index];
            if (stop.Frame == frame)
                return;
            stop.Frame = frame;
            _data[index] = stop;
            OnEdited();
        }

        private void SetStopColor(int index, Color color)
        {
            var stop = _data[index];
            if (stop.Value == color)
                return;
            stop.Value = color;
            _data[index] = stop;
            OnEdited();
        }

        private void UpdateControls()
        {
            var count = _data.Count;

            // Remove unused stops
            while (_stops.Count > count)
            {
                var last = _stops.Count - 1;
                if (_selected == _stops[last])
                    _selected = null;
                _stops[last].Dispose();
                _stops.RemoveAt(last);
            }

            // Add missing stops
            while (_stops.Count < count)
            {
                var stop = new StopControl
                {
                    AutoFocus = false,
                    Gradient = this,
                    Size = new Float2(10.0f, 10.0f),
                    Parent = this,
                };
                _stops.Add(stop);
            }

            // Update stops
            var scale = _scale;
            var height = Height;
            for (var i = 0; i < count; i++)
            {
                var control = _stops[i];
                var stop = _data[i];
                control.Location = new Float2(stop.Frame * scale - control.Width * 0.5f, (height - control.Height) * 0.5f);
                control.Index = i;
                control.TooltipText = stop.Value + " at frame " + stop.Frame;
            }
        }

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
        public override void Draw()
        {
            // Push clipping mask
            GetDesireClientArea(out var clientArea);
            Render2D.PushClip(ref clientArea);

            var style = Style.Current;
            var bounds = new Rectangle(Float2.Zero, Size);
            var count = _data.Count;
            if (count == 0)
            {
                //Render2D.FillRectangle(bounds, Color.Black);
            }
            else if (count == 1)
            {
                Render2D.FillRectangle(bounds, _data[0].Value);
            }
            else
            {
                var prevStop = _data[0];
                var scale = _scale;
                var width = Width;
                var height = Height;

                if (prevStop.Frame > 0.0f)
                {
                    Render2D.FillRectangle(new Rectangle(Float2.Zero, prevStop.Frame * scale, height), prevStop.Value);
                }

                for (int i = 1; i < count; i++)
                {
                    var curStop = _data[i];

                    Render2D.FillRectangle(new Rectangle(prevStop.Frame * scale, 0, (curStop.Frame - prevStop.Frame) * scale, height), prevStop.Value, curStop.Value, curStop.Value, prevStop.Value);

                    prevStop = curStop;
                }

                if (prevStop.Frame * scale < width)
                {
                    Render2D.FillRectangle(new Rectangle(prevStop.Frame * scale, 0, width - prevStop.Frame * scale, height), prevStop.Value);
                }
            }
            Render2D.DrawRectangle(bounds, IsMouseOver ? style.BackgroundHighlighted : style.Background);

            DrawChildren();

            // Pop clipping mask
            Render2D.PopClip();
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (base.OnMouseDoubleClick(location, button))
                return true;

            if (button == MouseButton.Left)
            {
                // Add stop
                var frame = (int)(location.X / _scale);
                var stops = new List<Stop>(Stops);
                var leftIdx = stops.FindLastIndex(x => x.Frame < frame);
                var rightIdx = stops.FindIndex(x => x.Frame > frame);
                var stop = new Stop
                {
                    Frame = frame,
                    Value = Color.White,
                };
                if (leftIdx != -1 && rightIdx != -1)
                {
                    var left = stops[leftIdx];
                    var right = stops[rightIdx];
                    float alpha = (float)(frame - left.Frame) / (right.Frame - left.Frame);
                    stop.Value = Color.Lerp(left.Value, right.Value, alpha);
                    stops.Insert(leftIdx + 1, stop);
                }
                else if (leftIdx != -1)
                {
                    stop.Value = stops[leftIdx].Value;
                    stops.Insert(leftIdx + 1, stop);
                }
                else if (rightIdx != -1)
                {
                    stop.Value = stops[rightIdx].Value;
                    stops.Insert(rightIdx, stop);
                }
                else
                {
                    stops.Add(stop);
                }
                Stops = stops;
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            SetUpdate(ref _update, null);
            _stops.Clear();
            _data.Clear();
            _stops = null;
            _data = null;

            base.OnDestroy();
        }
    }
}
