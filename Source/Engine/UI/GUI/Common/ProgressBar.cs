// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Progress bar control shows visual progress of the action or set of actions.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    public class ProgressBar : Control
    {
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
        /// <value>
        ///   <c>true</c> if use progress value smoothing; otherwise, <c>false</c>.
        /// </value>
        public bool UseSmoothing => !Mathf.IsZero(SmoothingScale);

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

                    // Check if skip smoothing
                    if (!UseSmoothing)
                    {
                        _current = _value;
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets the margin for the progress bar rectangle within the control bounds.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The margin for the progress bar rectangle within the control bounds.")]
        public Margin BarMargin { get; set; }

        /// <summary>
        /// Gets or sets the color of the progress bar rectangle.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color of the progress bar rectangle.")]
        public Color BarColor { get; set; }

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
            bool isDeltaSlow = deltaTime > (1 / 20.0f);

            // Ensure progress bar is visible
            if (Visible)
            {
                // Value smoothing
                if (Mathf.Abs(_current - _value) > 0.01f)
                {
                    // Lerp or not if running slow
                    float value;
                    if (!isDeltaSlow && UseSmoothing)
                        value = Mathf.Lerp(_current, _value, Mathf.Saturate(deltaTime * 5.0f * SmoothingScale));
                    else
                        value = _value;
                    _current = value;
                }
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            float progressNormalized = (_current - _minimum) / _maximum;
            if (progressNormalized > 0.001f)
            {
                var barRect = new Rectangle(0, 0, Width * progressNormalized, Height);
                BarMargin.ShrinkRectangle(ref barRect);
                Render2D.FillRectangle(barRect, BarColor);
            }
        }
    }
}
