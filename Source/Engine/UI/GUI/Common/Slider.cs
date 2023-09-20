using System;

namespace FlaxEngine.GUI;

/// <summary>
/// The slider control.
/// </summary>
public class Slider : ContainerControl
{
    /// <summary>
    /// The minimum value.
    /// </summary>
    protected float _minimum;

    /// <summary>
    /// The maximum value.
    /// </summary>
    protected float _maximum = 100f;

    /// <summary>
    /// Gets or sets the minimum value.
    /// </summary>
    [EditorOrder(20), Tooltip("The minimum value.")]
    public float Minimum
    {
        get => _minimum;
        set
        {
            if (value > _maximum)
                throw new ArgumentOutOfRangeException();
            if (WholeNumbers)
                value = Mathf.RoundToInt(value);
            _minimum = value;
            if (Value < _minimum)
                Value = _minimum;
        }
    }

    /// <summary>
    /// Gets or sets the maximum value.
    /// </summary>
    [EditorOrder(30), Tooltip("The maximum value.")]
    public float Maximum
    {
        get => _maximum;
        set
        {
            if (value < _minimum || Mathf.IsZero(value))
                throw new ArgumentOutOfRangeException();
            if (WholeNumbers)
                value = Mathf.RoundToInt(value);
            _maximum = value;
            if (Value > _maximum)
                Value = _maximum;
        }
    }

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
    public Float2 ThumbCenter => new(_thumbCenter, Height / 2);

    /// <summary>
    /// The local position of the beginning of the track.
    /// </summary>
    [HideInEditor]
    public Float2 TrackBeginning => new(_thumbSize.X / 2, Height / 2);

    /// <summary>
    /// The local position of the end of the track.
    /// </summary>
    [HideInEditor]
    public Float2 TrackEnd => new(Width - _thumbSize.X / 2, Height / 2);
    
    /// <summary>
    /// The height of the track.
    /// </summary>
    [EditorOrder(40), Tooltip("The track height.")]
    public int TrackHeight { get; set; } = 2;

    /// <summary>
    /// The thumb size.
    /// </summary>
    [EditorOrder(41), Tooltip("The size of the thumb.")]
    public Float2 ThumbSize {  
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
    /// Gets the size of the track.
    /// </summary>
    private float TrackWidth => Width;

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
        float trackSize = TrackWidth;
        float range = Maximum - Minimum;
        float pixelRange = trackSize - _thumbSize.X;
        float perc = (_value - Minimum) / range;
        float thumbPosition = (int)(perc * pixelRange);
        _thumbCenter = thumbPosition + _thumbSize.X / 2;
        _thumbRect = new Rectangle(thumbPosition, (Height - _thumbSize.Y) / 2, _thumbSize.X, _thumbSize.Y);
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
        
        // Draw track line
        //var lineRect = new Rectangle(4, (Height - TrackHeight) / 2, Width - 8, TrackHeight);
        var lineRect = new Rectangle(_thumbSize.X / 2, (Height - TrackHeight) / 2, Width - _thumbSize.X, TrackHeight);
        if (TrackBrush != null)
            TrackBrush.Draw(lineRect, TrackLineColor);
        else
            Render2D.FillRectangle(lineRect, TrackLineColor);
        
        // Draw track fill
        if (FillTrack)
        {
            var fillLineRect = new Rectangle(_thumbSize.X / 2, (Height - TrackHeight - 2) / 2, Width - (Width - _thumbCenter) - _thumbSize.X / 2, TrackHeight + 2);
            Render2D.PushClip(ref fillLineRect);
            if (FillTrackBrush != null)
                FillTrackBrush.Draw(lineRect, TrackFillLineColor);
            else
                Render2D.FillRectangle(lineRect, TrackFillLineColor);
            Render2D.PopClip();
        }
        
        // Draw thumb
        var thumbColor = _isSliding ? ThumbColorSelected : (_mouseOverThumb ? ThumbColorHighlighted : ThumbColor);
        if (ThumbBrush != null)
            ThumbBrush.Draw(_thumbRect, thumbColor);
        else
            Render2D.FillRectangle(_thumbRect, thumbColor);
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
            float mousePosition = location.X;

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
                Value += (mousePosition < _thumbCenter ? -1 : 1) * 10;
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
            Value = Mathf.Remap(slidePosition.X, 4, TrackWidth - 4, Minimum, Maximum);
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
