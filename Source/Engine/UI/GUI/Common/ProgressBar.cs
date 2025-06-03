// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Progress bar control shows visual progress of the action or set of actions.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [ActorToolbox("GUI")]
    public class ProgressBar : ContainerControl
    {
        /// <summary>
        /// The method used to effect the bar.
        /// </summary>
        public enum BarMethod
        {
            /// <summary>
            /// Stretch the bar.
            /// </summary>
            Stretch,
            
            /// <summary>
            /// Clip the bar.
            /// </summary>
            Clip,
        }
        
        /// <summary>
        /// The origin to move the progress bar to.
        /// </summary>
        public enum BarOrigin
        {
            /// <summary>
            /// Move the bar horizontally to the left.
            /// </summary>
            HorizontalLeft,
            
            /// <summary>
            /// Move the bar horizontally to the right.
            /// </summary>
            HorizontalRight,
            
            /// <summary>
            /// Move the bar vertically up.
            /// </summary>
            VerticalTop,
            
            /// <summary>
            /// Move the bar vertically down.
            /// </summary>
            VerticalBottom,
        }

        /// <summary>
        /// The value.
        /// </summary>
        protected float _value;

        /// <summary>
        /// The current value (used to apply smooth progress changes).
        /// </summary>
        protected float _current;

        /// <summary>
        /// The minimum progress value.
        /// </summary>
        protected float _minimum;

        /// <summary>
        /// The maximum progress value.
        /// </summary>
        protected float _maximum = 100;

        /// <summary>
        /// Gets or sets the value smoothing scale (0 to not use it).
        /// </summary>
        [EditorOrder(40), Limit(0, 100, 0.1f), Tooltip("The value smoothing scale (0 to not use it).")]
        public float SmoothingScale { get; set; } = 1;

        /// <summary>
        /// Gets a value indicating whether use progress value smoothing.
        /// </summary>
        public bool UseSmoothing => !Mathf.IsZero(SmoothingScale);

        /// <summary>
        /// The method used to effect the bar.
        /// </summary>
        [EditorOrder(41), Tooltip("The method used to effect the bar.")]
        public BarMethod Method = BarMethod.Stretch;
        
        /// <summary>
        /// The origin or where the bar decreases to.
        /// </summary>
        [EditorOrder(42), Tooltip("The origin or where the bar decreases to.")]
        public BarOrigin Origin = BarOrigin.HorizontalLeft;

        /// <summary>
        /// Gets or sets the minimum value.
        /// </summary>
        [EditorOrder(20), Tooltip("The minimum progress value.")]
        public float Minimum
        {
            get => _minimum;
            set
            {
                if (value > _maximum)
                    throw new ArgumentOutOfRangeException();
                _minimum = value;
                if (Value < _minimum)
                    Value = _minimum;
            }
        }

        /// <summary>
        /// Gets or sets the maximum value.
        /// </summary>
        [EditorOrder(30), Tooltip("The maximum progress value.")]
        public float Maximum
        {
            get => _maximum;
            set
            {
                if (value < _minimum || Mathf.IsZero(value))
                    throw new ArgumentOutOfRangeException();
                _maximum = value;
                if (Value > _maximum)
                    Value = _maximum;
            }
        }

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        [EditorOrder(10), Tooltip("The current progress value.")]
        public float Value
        {
            get => _value;
            set
            {
                value = Mathf.Clamp(value, _minimum, _maximum);
                if (!Mathf.NearEqual(value, _value))
                {
                    _value = value;
                    if (!UseSmoothing || _firstUpdate)
                    {
                        _current = _value;
                        if (_firstUpdate)
                            _firstUpdate = false;
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets the margin for the progress bar rectangle within the control bounds.
        /// </summary>
        [EditorDisplay("Bar Style"), EditorOrder(2011), Tooltip("The margin for the progress bar rectangle within the control bounds.")]
        public Margin BarMargin { get; set; }

        /// <summary>
        /// Gets or sets the color of the progress bar rectangle.
        /// </summary>
        [EditorDisplay("Bar Style"), EditorOrder(2010), Tooltip("The color of the progress bar rectangle."), ExpandGroups]
        public Color BarColor { get; set; }

        /// <summary>
        /// Gets or sets the brush used for progress bar drawing.
        /// </summary>
        [EditorDisplay("Bar Style"), EditorOrder(2012), Tooltip("The brush used for progress bar drawing.")]
        public IBrush BarBrush { get; set; }

        // Used to remove initial lerp from the value on play.
        private bool _firstUpdate = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProgressBar"/> class.
        /// </summary>
        public ProgressBar()
        : this(0, 0, 120)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ProgressBar"/> class.
        /// </summary>
        public ProgressBar(float x, float y, float width, float height = 28)
        : base(x, y, width, height)
        {
            AutoFocus = false;

            var style = Style.Current;
            BackgroundColor = style.Background;
            BarColor = style.ProgressNormal;
            BarMargin = new Margin(1);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (Visible)
            {
                // Value smoothing
                var value = _value;
                if (Mathf.Abs(_current - _value) > 0.01f)
                {
                    // Lerp or not if running slow
                    bool isDeltaSlow = deltaTime > (1 / 20.0f);
                    if (!isDeltaSlow && UseSmoothing)
                        value = Mathf.Lerp(_current, _value, Mathf.Saturate(deltaTime * 5.0f * SmoothingScale));
                    _current = value;
                }
                else
                {
                    _current = _value;
                }
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            base.DrawSelf();

            float progressNormalized = Mathf.InverseLerp(_minimum, _maximum, _current);
            if (progressNormalized > 0.001f)
            {
                Rectangle barRect = new Rectangle(0, 0, Width * progressNormalized, Height);
                switch (Origin)
                {
                case BarOrigin.HorizontalLeft:
                    break;
                case BarOrigin.HorizontalRight:
                    barRect = new Rectangle(Width - Width * progressNormalized, 0, Width * progressNormalized, Height);
                    break;
                case BarOrigin.VerticalTop:
                    barRect = new Rectangle(0, 0, Width, Height * progressNormalized);
                    break;
                case BarOrigin.VerticalBottom:
                    barRect = new Rectangle(0, Height - Height * progressNormalized, Width, Height * progressNormalized);
                    break;
                default: break;
                }

                switch (Method)
                {
                case BarMethod.Stretch:
                    BarMargin.ShrinkRectangle(ref barRect);
                    if (BarBrush != null)
                        BarBrush.Draw(barRect, BarColor);
                    else
                        Render2D.FillRectangle(barRect, BarColor);
                    break;
                case BarMethod.Clip:
                    var rect = new Rectangle(0, 0, Width, Height);
                    BarMargin.ShrinkRectangle(ref rect);
                    Render2D.PushClip(ref barRect);
                    if (BarBrush != null)
                        BarBrush.Draw(rect, BarColor);
                    else
                        Render2D.FillRectangle(rect, BarColor);
                    Render2D.PopClip();
                    break;
                default: break;
                }
            }
        }
    }
}
