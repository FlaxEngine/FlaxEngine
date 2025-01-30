// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Scroll Bars base class - allows to scroll contents of the GUI panel.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [HideInEditor]
    public abstract class ScrollBar : Control
    {
        /// <summary>
        /// The default size.
        /// </summary>
        public const int DefaultSize = 14;

        /// <summary>
        /// The default minimum opacity.
        /// </summary>
        public const float DefaultMinimumOpacity = 0.75f;

        // Scrolling

        private float _clickChange = 20, _scrollChange = 50;
        private float _minimum, _maximum = 100;
        private float _startValue, _value, _targetValue;
        private readonly Orientation _orientation;
        private RootControl.UpdateDelegate _update;

        // Input

        private float _mouseOffset;

        // Thumb data

        private Rectangle _thumbRect, _trackRect;
        private bool _thumbClicked;
        private float _thumbCenter, _thumbSize;

        // Smoothing

        private float _thumbOpacity = DefaultMinimumOpacity;
        private float _scrollAnimationProgress = 0f;

        /// <summary>
        /// Gets the orientation.
        /// </summary>
        public Orientation Orientation => _orientation;

        /// <summary>
        /// Gets or sets the thumb box thickness.
        /// </summary>
        public float ThumbThickness { get; set; } = 8;

        /// <summary>
        /// Gets or sets the track line thickness.
        /// </summary>
        public float TrackThickness { get; set; } = 2.0f;

        /// <summary>
        /// The maximum time it takes to animate from current to target scroll position 
        /// </summary>
        public float ScrollAnimationDuration { get; set; } = 0.18f;

        /// <summary>
        /// Gets a value indicating whether use scroll value smoothing.
        /// </summary>
        public bool UseSmoothing => EnableSmoothing && !Mathf.IsZero(ScrollAnimationDuration);

        /// <summary>
        /// Enables scroll smoothing
        /// </summary>
        public bool EnableSmoothing { get; set; } = true;

        /// <summary>
        /// The track color.
        /// </summary>
        public Color TrackColor;

        /// <summary>
        /// The thumb color.
        /// </summary>
        public Color ThumbColor;

        /// <summary>
        /// The selected thumb color.
        /// </summary>
        public Color ThumbSelectedColor;

        /// <summary>
        /// Gets or sets the minimum value.
        /// </summary>
        public float Minimum
        {
            get => _minimum;
            set
            {
                if (value > _maximum)
                    throw new ArgumentOutOfRangeException();
                _minimum = value;
                UpdateThumb();
                if (Value < _minimum)
                    Value = _minimum;
            }
        }

        /// <summary>
        /// Gets or sets the maximum value.
        /// </summary>
        public float Maximum
        {
            get => _maximum;
            set
            {
                if (value < _minimum)
                    throw new ArgumentOutOfRangeException();
                _maximum = value;
                UpdateThumb();
                if (Value > _maximum)
                    Value = _maximum;
            }
        }

        /// <summary>
        /// Gets or sets the scroll value (current, smooth).
        /// </summary>
        public float Value
        {
            get => _value;
            set
            {
                value = Mathf.Clamp(value, _minimum, _maximum);
                if (!Mathf.NearEqual(value, _targetValue))
                {
                    _targetValue = value;
                    _startValue = _value;
                    _scrollAnimationProgress = 0f;

                    // Check if skip smoothing
                    if (!UseSmoothing)
                    {
                        _value = value;
                        _startValue = value;
                        _scrollAnimationProgress = 1f;
                        OnValueChanged();
                    }
                    else
                    {
                        SetUpdate(ref _update, OnUpdate);
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets the target value (target, not smooth).
        /// </summary>
        public float TargetValue
        {
            get => _targetValue;
            set
            {
                value = Mathf.Clamp(value, _minimum, _maximum);
                if (!Mathf.NearEqual(value, _targetValue))
                {
                    _targetValue = value;
                    _value = value;
                    SetUpdate(ref _update, null);
                    OnValueChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets the speed for the scroll on mouse wheel.
        /// </summary>
        public float ScrollSpeedWheel
        {
            get => _scrollChange;
            set => _scrollChange = value;
        }

        /// <summary>
        /// Gets or sets the speed for the scroll on mouse click.
        /// </summary>
        public float ScrollSpeedClick
        {
            get => _clickChange;
            set => _clickChange = value;
        }

        /// <summary>
        /// Gets the value slow down.
        /// </summary>
        public float ValueSlowDown => _targetValue - _value;

        /// <summary>
        /// Gets a value indicating whether thumb is being clicked (scroll bar is in use).
        /// </summary>
        public bool IsThumbClicked => _thumbClicked;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// Enables/disabled scrolling by user.
        /// </summary>
        [NoSerialize, HideInEditor]
        public bool ThumbEnabled = true;

        /// <summary>
        /// Gets the size of the track.
        /// </summary>
        protected abstract float TrackSize { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="ScrollBar"/> class.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        protected ScrollBar(Orientation orientation)
        {
            AutoFocus = false;

            _orientation = orientation;
            var style = Style.Current;
            TrackColor = style.BackgroundHighlighted;
            ThumbColor = style.BackgroundNormal;
            ThumbSelectedColor = style.BackgroundSelected;
        }

        /// <summary>
        /// Cuts the scroll bar value smoothing and imminently goes to the target scroll value.
        /// </summary>
        public void FastScroll()
        {
            if (!Mathf.NearEqual(_value, _targetValue))
            {
                _value = _targetValue = _startValue;
                _scrollAnimationProgress = 0f;
                SetUpdate(ref _update, null);
                OnValueChanged();
            }
        }

        /// <summary>
        /// Scrolls the view to the desire range (favors minimum value if cannot cover whole range in a bounds).
        /// </summary>
        /// <param name="min">The view minimum.</param>
        /// <param name="max">The view maximum.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        public void ScrollViewTo(float min, float max, bool fastScroll = false)
        {
            // Check if we need to change view
            float viewMin = _value;
            float viewSize = TrackSize;
            float viewMax = viewMin + viewSize;
            if (Mathf.IsNotInRange(min, viewMin, viewMax))
            {
                if (fastScroll)
                    TargetValue = min;
                else
                    Value = min;
            }
            /*else if (Mathf.IsNotInRange(max, viewMin, viewMax))
            {
                Value = max - viewSize;
            }*/
        }

        private void UpdateThumb()
        {
            // Cache data
            var width = Width;
            var height = Height;
            float trackSize = TrackSize;
            float range = _maximum - _minimum;
            _thumbSize = Mathf.Min(trackSize - 10, Mathf.Max(trackSize / range * 100.0f, 50.0f));
            float pixelRange = trackSize - _thumbSize;
            float percentage = (_value - _minimum) / range;
            float thumbPosition = (int)(percentage * pixelRange);
            _thumbCenter = thumbPosition + _thumbSize / 2;
            _thumbRect = _orientation == Orientation.Vertical
                         ? new Rectangle((width - ThumbThickness) / 2, thumbPosition + 4, ThumbThickness, _thumbSize - 8)
                         : new Rectangle(thumbPosition + 4, (height - ThumbThickness) / 2, _thumbSize - 8, ThumbThickness);
            _trackRect = _orientation == Orientation.Vertical
                         ? new Rectangle((width - TrackThickness) / 2, 4, TrackThickness, height - 8)
                         : new Rectangle(4, (height - TrackThickness) / 2, width - 8, TrackThickness);
        }

        private void EndTracking()
        {
            // Check flag
            if (_thumbClicked)
            {
                // Clear flag
                _thumbClicked = false;

                // End capturing mouse
                EndMouseCapture();
            }
        }

        internal void Reset()
        {
            _value = _targetValue = _startValue = 0;
            _scrollAnimationProgress = 0f;
        }

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            UpdateThumb();

            ValueChanged?.Invoke();
        }

        private void OnUpdate(float deltaTime)
        {
            bool isDeltaSlow = deltaTime > (1 / 20.0f);

            // Opacity smoothing
            float targetOpacity = IsMouseOver ? 1.0f : DefaultMinimumOpacity;
            _thumbOpacity = isDeltaSlow ? targetOpacity : Mathf.Lerp(_thumbOpacity, targetOpacity, deltaTime * 10.0f);
            bool needUpdate = Mathf.Abs(_thumbOpacity - targetOpacity) > 0.001f;

            // Ensure scroll bar is visible and smoothing is required
            if (Visible && Mathf.Abs(_targetValue - _value) > 0.01f)
            {
                // Interpolate or not if running slow
                float value;
                if (!isDeltaSlow && UseSmoothing)
                {
                    // percentage of scroll from 0 to _scrollChange, ex. 0.5 at _scrollChange / 2
                    var minScrollChangeRatio = Mathf.Clamp(Mathf.Abs(_targetValue - _startValue) / _scrollChange, 0, 1);
                        
                    // shorten the duration if we scrolled less than _scrollChange
                    var actualDuration = ScrollAnimationDuration * minScrollChangeRatio;
                    var step = deltaTime / actualDuration;

                    var progress = _scrollAnimationProgress;
                    progress = Mathf.Clamp(progress + step, 0, 1);
                            
                    // https://easings.net/#easeOutSine
                    var easedProgress = Mathf.Sin((progress * Mathf.Pi) / 2);
                    if (progress >= 1.0f)
                        easedProgress = 1.0f;
                    value = Mathf.Lerp(_startValue, _targetValue, easedProgress);

                    _scrollAnimationProgress = progress;
                }
                else
                {
                    value = _targetValue;
                    _startValue = _targetValue;
                    _scrollAnimationProgress = 0f;
                }
                
                _value = value;
                OnValueChanged();
                needUpdate = true;
            }

            // End updating if all animations are done
            if (!needUpdate)
            {
                SetUpdate(ref _update, null);
            }
        }

        /// <summary>
        /// Sets the scroll range (min and max at once).
        /// </summary>
        /// <param name="minimum">The minimum scroll range value (see <see cref="Minimum"/>).</param>
        /// <param name="maximum">The maximum scroll range value (see <see cref="Minimum"/>).</param>
        public void SetScrollRange(float minimum, float maximum)
        {
            if (minimum > maximum)
                throw new ArgumentOutOfRangeException();

            _minimum = minimum;
            _maximum = maximum;

            if (Value < minimum)
                Value = minimum;
            else if (Value > maximum)
                Value = maximum;

            UpdateThumb();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var style = Style.Current;
            Render2D.FillRectangle(_trackRect, TrackColor * _thumbOpacity);
            Render2D.FillRectangle(_thumbRect, (_thumbClicked ? ThumbSelectedColor : ThumbColor) * _thumbOpacity);
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            EndTracking();

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_thumbClicked)
            {
                var slidePosition = location + Root.TrackingMouseOffset;
                if (Parent is ScrollableControl panel)
                    slidePosition += panel.ViewOffset; // Hardcoded fix
                float mousePosition = _orientation == Orientation.Vertical ? slidePosition.Y : slidePosition.X;

                float percentage = (mousePosition - _mouseOffset - _thumbSize / 2) / (TrackSize - _thumbSize);
                TargetValue = _minimum + percentage * (_maximum - _minimum);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (ThumbEnabled)
            {
                // Scroll
                Value = _targetValue - delta * _scrollChange;
            }
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && ThumbEnabled)
            {
                // Remove focus
                var root = Root;
                root.FocusedControl?.Defocus();

                float mousePosition = _orientation == Orientation.Vertical ? location.Y : location.X;

                if (_thumbRect.Contains(ref location))
                {
                    // Start moving thumb
                    _thumbClicked = true;
                    _mouseOffset = mousePosition - _thumbCenter;

                    // Start capturing mouse
                    StartMouseCapture();
                }
                else
                {
                    // Click change
                    Value = _value + (mousePosition < _thumbCenter ? -1 : 1) * _clickChange;
                }
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            EndTracking();

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            EndTracking();
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            UpdateThumb();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            SetUpdate(ref _update, OnUpdate);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            SetUpdate(ref _update, OnUpdate);
        }

        /// <inheritdoc />
        protected override void AddUpdateCallbacks(RootControl root)
        {
            base.AddUpdateCallbacks(root);

            if (_update != null)
                root.UpdateCallbacksToAdd.Add(_update);
        }

        /// <inheritdoc />
        protected override void RemoveUpdateCallbacks(RootControl root)
        {
            base.RemoveUpdateCallbacks(root);

            if (_update != null)
                root.UpdateCallbacksToRemove.Add(_update);
        }
    }
}
