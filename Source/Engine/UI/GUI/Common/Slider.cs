// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI;

/// <summary>
/// The slider control.
/// </summary>
[ActorToolbox("GUI")]
public class Slider : ContainerControl
{
    /// <summary>
    /// The slider direction
    /// </summary>
    public enum SliderDirection
    {
        /// <summary>
        /// Slider direction, horizontal right
        /// </summary>
        HorizontalRight,

        /// <summary>
        /// Slider direction, horizontal left
        /// </summary>
        HorizontalLeft,

        /// <summary>
        /// Slider direction, vertical up
        /// </summary>
        VerticalUp,

        /// <summary>
        /// Slider direction, vertical down
        /// </summary>
        VerticalDown,
    }

    /// <summary>
    /// The minimum value.
    /// </summary>
    protected float _minimum;

    /// <summary>
    /// The maximum value.
    /// </summary>
    protected float _maximum = 100;

    /// <summary>
    /// Gets or sets the maximum value.
    /// </summary>
    [EditorOrder(30), Tooltip("The maximum value.")]
    public float Maximum
    {
        get => _maximum;
        set
        {
            if (WholeNumbers)
                value = Mathf.RoundToInt(value);
            _maximum = value;
            if (Value > _maximum)
                Value = _maximum;
        }
    }

    /// <summary>
    /// Gets or sets the minimum value.
    /// </summary>
    [EditorOrder(20), Tooltip("The minimum value.")]
    public float Minimum
    {
        get => _minimum;
        set
        {
            if (WholeNumbers)
                value = Mathf.RoundToInt(value);
            _minimum = value;
            if (Value < _minimum)
                Value = _minimum;
        }
    }

    /// <summary>
    /// Gets or sets the slider direction.
    /// </summary>
    [EditorOrder(40), Tooltip("Slider Direction.")]
    public SliderDirection Direction
    {
        get => _direction;
        set
        {
            _direction = value;
            UpdateThumb();
        }
    }

    private SliderDirection _direction = SliderDirection.HorizontalRight;
    private float _value = 100f;
    private Rectangle _thumbRect;
    private float _thumbCenter;
    private Float2 _thumbSize = new Float2(16, 16);
    private bool _isSliding;
    private bool _mouseOverThumb;

    /// <summary>
    /// Gets or sets the value (normalized to range 0-100).
    /// </summary>
    [EditorOrder(10), Tooltip("The current value.")]
    public float Value
    {
        get => _value;
        set
        {
            value = Mathf.Clamp(value, Minimum, Maximum);
            if (WholeNumbers)
                value = Mathf.RoundToInt(value);
            if (!Mathf.NearEqual(value, _value))
            {
                _value = value;

                // Update
                UpdateThumb();
                ValueChanged?.Invoke();
            }
        }
    }

    /// <summary>
    /// The local position of the thumb center
    /// </summary>
    [HideInEditor]
    public Float2 ThumbCenter => (Direction is SliderDirection.HorizontalLeft or SliderDirection.HorizontalRight) ? new Float2(_thumbCenter, Height / 2) : new Float2(Width / 2, _thumbCenter);

    /// <summary>
    /// The local position of the beginning of the track.
    /// </summary>
    [HideInEditor]
    public Float2 TrackBeginning
    {
        get
        {
            switch (Direction)
            {
            case SliderDirection.HorizontalRight: return new Float2(_thumbSize.X / 2, Height / 2);
            case SliderDirection.HorizontalLeft: return new Float2(Width - _thumbSize.X / 2, Height / 2);
            case SliderDirection.VerticalUp: return new Float2(Width / 2, Height - _thumbSize.Y / 2);
            case SliderDirection.VerticalDown: return new Float2(Width / 2, _thumbSize.Y / 2);
            default: break;
            }
            return Float2.Zero;
        }
    }

    /// <summary>
    /// The local position of the end of the track.
    /// </summary>
    [HideInEditor]
    public Float2 TrackEnd
    {
        get
        {
            switch (Direction)
            {
            case SliderDirection.HorizontalRight: return new Float2(Width - _thumbSize.X / 2, Height / 2);
            case SliderDirection.HorizontalLeft: return new Float2(_thumbSize.X / 2, Height / 2);
            case SliderDirection.VerticalUp: return new Float2(Width / 2, _thumbSize.Y / 2);
            case SliderDirection.VerticalDown: return new Float2(Width / 2, Height - _thumbSize.Y / 2);
            default: break;
            }
            return Float2.Zero;
        }
    }

    /// <summary>
    /// The height of the track.
    /// </summary>
    [EditorOrder(40), Tooltip("The track height.")]
    public int TrackThickness { get; set; } = 2;

    /// <summary>
    /// The thumb size.
    /// </summary>
    [EditorOrder(41), Tooltip("The size of the thumb.")]
    public Float2 ThumbSize
    {
        get => _thumbSize;
        set
        {
            _thumbSize = value;
            UpdateThumb();
        }
    }

    /// <summary>
    /// Whether to fill the track.
    /// </summary>
    [EditorOrder(42), Tooltip("Fill the track.")]
    public bool FillTrack = true;

    /// <summary>
    /// Whether to use whole numbers.
    /// </summary>
    [EditorOrder(43), Tooltip("Use whole numbers.")]
    public bool WholeNumbers = false;

    /// <summary>
    /// The color of the slider track line
    /// </summary>
    [EditorDisplay("Track Style"), EditorOrder(2010), Tooltip("The color of the slider track line."), ExpandGroups]
    public Color TrackLineColor { get; set; }

    /// <summary>
    /// The color of the slider fill track line
    /// </summary>
    [EditorDisplay("Track Style"), EditorOrder(2011), VisibleIf(nameof(FillTrack)), Tooltip("The color of the slider fill track line.")]
    public Color TrackFillLineColor { get; set; }

    /// <summary>
    /// Gets the width of the track.
    /// </summary>
    private float TrackWidth => (Direction is SliderDirection.HorizontalLeft or SliderDirection.HorizontalRight) ? Width - _thumbSize.X : TrackThickness;

    /// <summary>
    /// Gets the height of the track.
    /// </summary>
    private float TrackHeight => (Direction is SliderDirection.HorizontalLeft or SliderDirection.HorizontalRight) ? TrackThickness : Height - _thumbSize.Y;

    /// <summary>
    /// Gets or sets the brush used for slider track drawing.
    /// </summary>
    [EditorDisplay("Track Style"), EditorOrder(2012), Tooltip("The brush used for slider track drawing.")]
    public IBrush TrackBrush { get; set; }

    /// <summary>
    /// Gets or sets the brush used for slider fill track drawing.
    /// </summary>
    [EditorDisplay("Track Style"), EditorOrder(2013), VisibleIf(nameof(FillTrack)), Tooltip("The brush used for slider fill track drawing.")]
    public IBrush FillTrackBrush { get; set; }

    /// <summary>
    /// The color of the slider thumb when it's not selected.
    /// </summary>
    [EditorDisplay("Thumb Style"), EditorOrder(2030), Tooltip("The color of the slider thumb when it's not selected."), ExpandGroups]
    public Color ThumbColor { get; set; }

    /// <summary>
    /// The color of the slider thumb when it's highlighted.
    /// </summary>
    [EditorDisplay("Thumb Style"), EditorOrder(2031), Tooltip("The color of the slider thumb when it's highlighted.")]
    public Color ThumbColorHighlighted { get; set; }

    /// <summary>
    /// The color of the slider thumb when it's selected.
    /// </summary>
    [EditorDisplay("Thumb Style"), EditorOrder(2032), Tooltip("The color of the slider thumb when it's selected.")]
    public Color ThumbColorSelected { get; set; }

    /// <summary>
    /// Gets or sets the brush used for slider thumb drawing.
    /// </summary>
    [EditorDisplay("Thumb Style"), EditorOrder(2033), Tooltip("The brush of the slider thumb.")]
    public IBrush ThumbBrush { get; set; }

    /// <summary>
    /// Gets a value indicating whether user is using a slider.
    /// </summary>
    [HideInEditor]
    public bool IsSliding => _isSliding;

    /// <summary>
    /// Occurs when sliding starts.
    /// </summary>
    public event Action SlidingStart;

    /// <summary>
    /// Occurs when sliding ends.
    /// </summary>
    public event Action SlidingEnd;

    /// <summary>
    /// Occurs when value gets changed.
    /// </summary>
    public event Action ValueChanged;

    /// <summary>
    /// Initializes a new instance of the <see cref="Slider"/> class.
    /// </summary>
    public Slider()
    : this(120, 30)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Slider"/> class.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    public Slider(float width, float height)
    : base(0, 0, width, height)
    {
        var style = Style.Current;
        TrackLineColor = style.BackgroundHighlighted;
        TrackFillLineColor = style.LightBackground;
        ThumbColor = style.BackgroundNormal;
        ThumbColorSelected = style.BackgroundSelected;
        ThumbColorHighlighted = style.BackgroundHighlighted;
        UpdateThumb();
    }

    private void UpdateThumb()
    {
        // Cache data
        var isHorizontal = Direction is SliderDirection.HorizontalRight or SliderDirection.HorizontalLeft;
        float trackSize = isHorizontal ? Width : Height;
        float range = Maximum - Minimum;
        float pixelRange = trackSize - (isHorizontal ? _thumbSize.X : _thumbSize.Y);
        float perc = (_value - Minimum) / range;
        float thumbPosition = (int)(perc * pixelRange);
        switch (Direction)
        {
        case SliderDirection.HorizontalRight:
            _thumbCenter = thumbPosition + _thumbSize.X / 2;
            _thumbRect = new Rectangle(thumbPosition, (Height - _thumbSize.Y) / 2, _thumbSize.X, _thumbSize.Y);
            break;
        case SliderDirection.VerticalDown:
            _thumbCenter = thumbPosition + _thumbSize.Y / 2;
            _thumbRect = new Rectangle((Width - _thumbSize.X) / 2, thumbPosition, _thumbSize.X, _thumbSize.Y);
            break;
        case SliderDirection.HorizontalLeft:
            _thumbCenter = Width - thumbPosition - _thumbSize.X / 2;
            _thumbRect = new Rectangle(Width - thumbPosition - _thumbSize.X, (Height - _thumbSize.Y) / 2, _thumbSize.X, _thumbSize.Y);
            break;
        case SliderDirection.VerticalUp:
            _thumbCenter = Height - thumbPosition - _thumbSize.Y / 2;
            _thumbRect = new Rectangle((Width - _thumbSize.X) / 2, Height - thumbPosition - _thumbSize.Y, _thumbSize.X, _thumbSize.Y);
            break;
        default: break;
        }
    }

    private void EndSliding()
    {
        _isSliding = false;
        EndMouseCapture();
        SlidingEnd?.Invoke();
    }

    /// <inheritdoc />
    public override void Draw()
    {
        base.Draw();

        // Set rectangles
        var lineRect = new Rectangle(_thumbSize.X / 2, (Height - TrackThickness) / 2, Width - _thumbSize.X, TrackThickness);
        var fillLineRect = new Rectangle(_thumbSize.X / 2 - 1, (Height - TrackThickness - 2) / 2, Width - (Width - _thumbCenter) - _thumbSize.X / 2 + 1, TrackThickness + 2);
        switch (Direction)
        {
        case SliderDirection.HorizontalRight: break;
        case SliderDirection.VerticalDown:
            lineRect = new Rectangle((Width - TrackThickness) / 2, _thumbSize.Y / 2, TrackThickness, Height - _thumbSize.Y);
            fillLineRect = new Rectangle((Width - TrackThickness - 2) / 2, _thumbSize.Y / 2 - 1, TrackThickness + 2, Height - (Height - _thumbCenter) - _thumbSize.Y / 2 + 1);
            break;
        case SliderDirection.HorizontalLeft:
            fillLineRect = new Rectangle(Width - (Width - _thumbCenter) - 1, (Height - TrackThickness - 2) / 2, Width - _thumbCenter + 1, TrackThickness + 2);
            break;
        case SliderDirection.VerticalUp:
            lineRect = new Rectangle((Width - TrackThickness) / 2, _thumbSize.Y / 2, TrackThickness, Height - _thumbSize.Y);
            fillLineRect = new Rectangle((Width - TrackThickness - 2) / 2, Height - (Height - _thumbCenter) - 1, TrackThickness + 2, Height - _thumbCenter + 1);
            break;
        default: break;
        }

        // Draw track line
        if (TrackBrush != null)
            TrackBrush.Draw(lineRect, TrackLineColor);
        else
            Render2D.FillRectangle(lineRect, TrackLineColor);

        // Draw track fill
        if (FillTrack)
        {
            Render2D.PushClip(ref fillLineRect);
            if (FillTrackBrush != null)
                FillTrackBrush.Draw(lineRect, TrackFillLineColor);
            else
                Render2D.FillRectangle(lineRect, TrackFillLineColor);
            Render2D.PopClip();
        }

        // Draw thumb
        var thumbColorV = _isSliding ? ThumbColorSelected : (_mouseOverThumb ? ThumbColorHighlighted : ThumbColor);
        if (ThumbBrush != null)
            ThumbBrush.Draw(_thumbRect, thumbColorV);
        else
            Render2D.FillRectangle(_thumbRect, thumbColorV);
    }

    /// <inheritdoc />
    public override void OnLostFocus()
    {
        if (_isSliding)
        {
            EndSliding();
        }

        base.OnLostFocus();
    }

    /// <inheritdoc />
    public override bool OnMouseDown(Float2 location, MouseButton button)
    {
        if (button == MouseButton.Left)
        {
            Focus();
            float mousePosition = Direction is SliderDirection.HorizontalRight or SliderDirection.HorizontalLeft ? location.X : location.Y;

            if (_thumbRect.Contains(ref location))
            {
                // Start sliding
                _isSliding = true;
                StartMouseCapture();
                SlidingStart?.Invoke();
                return true;
            }
            else
            {
                // Click change
                switch (Direction)
                {
                case SliderDirection.HorizontalRight or SliderDirection.VerticalDown:
                    Value += (mousePosition < _thumbCenter ? -1 : 1) * 10;
                    break;
                case SliderDirection.HorizontalLeft or SliderDirection.VerticalUp:
                    Value -= (mousePosition < _thumbCenter ? -1 : 1) * 10;
                    break;
                default: break;
                }
            }
        }

        return base.OnMouseDown(location, button);
    }

    /// <inheritdoc />
    public override void OnMouseMove(Float2 location)
    {
        _mouseOverThumb = _thumbRect.Contains(location);
        if (_isSliding)
        {
            // Update sliding
            var slidePosition = location + Root.TrackingMouseOffset;
            switch (Direction)
            {
            case SliderDirection.HorizontalRight:
                Value = Mathf.Remap(slidePosition.X, 4, Width - 4, Minimum, Maximum);
                break;
            case SliderDirection.VerticalDown:
                Value = Mathf.Remap(slidePosition.Y, 4, Height - 4, Minimum, Maximum);
                break;
            case SliderDirection.HorizontalLeft:
                Value = Mathf.Remap(slidePosition.X, Width - 4, 4, Minimum, Maximum);
                break;
            case SliderDirection.VerticalUp:
                Value = Mathf.Remap(slidePosition.Y, Height - 4, 4, Minimum, Maximum);
                break;
            default: break;
            }
        }
        else
        {
            base.OnMouseMove(location);
        }
    }

    /// <inheritdoc />
    public override bool OnMouseUp(Float2 location, MouseButton button)
    {
        if (button == MouseButton.Left && _isSliding)
        {
            EndSliding();
            return true;
        }

        return base.OnMouseUp(location, button);
    }

    /// <inheritdoc />
    public override void OnEndMouseCapture()
    {
        // Check if was sliding
        if (_isSliding)
        {
            EndSliding();
        }
        else
        {
            base.OnEndMouseCapture();
        }
    }

    /// <inheritdoc />
    protected override void OnSizeChanged()
    {
        base.OnSizeChanged();

        UpdateThumb();
    }
}
