// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// Draws simple chart.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    internal class SingleChart : Control
    {
        internal const float DefaultHeight = TitleHeight + 60;
        private const float TitleHeight = 20;
        private const float PointsOffset = 4;
        private readonly SamplesBuffer<float> _samples;
        private string _sample;
        private int _selectedSampleIndex = -1;
        private bool _isSelecting;

        /// <summary>
        /// Gets or sets the chart title.
        /// </summary>
        public string Title { get; set; }

        /// <summary>
        /// Gets the index of the selected sample. Value -1 is used to indicate no selection (using the latest sample).
        /// </summary>
        public int SelectedSampleIndex
        {
            get => _selectedSampleIndex;
            set
            {
                value = Mathf.Clamp(value, -1, _samples.Count - 1);
                if (_selectedSampleIndex != value)
                {
                    _selectedSampleIndex = value;
                    _sample = _samples.Count == 0 ? string.Empty : FormatSample(_samples.Get(_selectedSampleIndex));

                    SelectedSampleChanged?.Invoke(_selectedSampleIndex);
                }
            }
        }

        /// <summary>
        /// Gets the selected sample value.
        /// </summary>
        public float SelectedSample => _samples.Get(_selectedSampleIndex);

        /// <summary>
        /// Occurs when selected sample gets changed.
        /// </summary>
        public event Action<int> SelectedSampleChanged;

        /// <summary>
        /// The handler function to format sample value for label text.
        /// </summary>
        public Func<float, string> FormatSample = (v) => v.ToString();

        /// <summary>
        /// Initializes a new instance of the <see cref="SingleChart"/> class.
        /// </summary>
        /// <param name="maxSamples">The maximum samples to collect.</param>
#if USE_PROFILER
        public SingleChart(int maxSamples = ProfilerMode.MaxSamples)
#else
        public SingleChart(int maxSamples = 600)
#endif
        : base(0, 0, 100, DefaultHeight)
        {
            _samples = new SamplesBuffer<float>(maxSamples);
            _sample = string.Empty;
        }

        /// <summary>
        /// Clears all the samples.
        /// </summary>
        public void Clear()
        {
            _samples.Clear();
            _sample = string.Empty;
        }

        /// <summary>
        /// Adds the sample value.
        /// </summary>
        /// <param name="value">The value.</param>
        public void AddSample(float value)
        {
            _samples.Add(value);
            _sample = FormatSample(value);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var style = Style.Current;
            float chartHeight = Height - TitleHeight;

            // Draw chart
            if (_samples.Count > 0)
            {
                var chartRect = new Rectangle(0, 0, Width, chartHeight);
                Render2D.PushClip(ref chartRect);

                if (_selectedSampleIndex != -1)
                {
                    float selectedX = Width - (_samples.Count - _selectedSampleIndex - 1) * PointsOffset;
                    Render2D.DrawLine(new Float2(selectedX, 0), new Float2(selectedX, chartHeight), style.Foreground, 1.5f);
                }

                int samplesInViewCount = Math.Min((int)(Width / PointsOffset), _samples.Count) - 1;
                float maxValue = _samples[_samples.Count - 1];
                for (int i = 2; i < samplesInViewCount; i++)
                {
                    maxValue = Mathf.Max(maxValue, _samples[_samples.Count - i]);
                }

                Color chartColor = style.BackgroundSelected;
                var chartRoot = chartRect.BottomRight;
                float samplesRange = maxValue * 1.1f;
                float samplesCoeff = -chartHeight / samplesRange;
                var posPrev = chartRoot + new Float2(0, _samples.Last * samplesCoeff);
                float posX = 0;

                for (int i = _samples.Count - 1; i >= 0; i--)
                {
                    float sample = _samples[i];
                    var pos = chartRoot + new Float2(posX, sample * samplesCoeff);
                    Render2D.DrawLine(posPrev, pos, chartColor);
                    posPrev = pos;
                    posX -= PointsOffset;
                }

                Render2D.PopClip();
            }

            // Draw title
            var headerRect = new Rectangle(0, chartHeight, Width, TitleHeight);
            var headerTextRect = new Rectangle(2, chartHeight, Width - 4, TitleHeight);
            Render2D.FillRectangle(headerRect, style.BackgroundNormal);
            Render2D.DrawText(style.FontMedium, Title, headerTextRect, style.ForegroundGrey, TextAlignment.Near, TextAlignment.Center);
            Render2D.DrawText(style.FontMedium, _sample, headerTextRect, style.Foreground, TextAlignment.Far, TextAlignment.Center);
        }

        private void OnClick(ref Float2 location)
        {
            float samplesWidth = _samples.Count * PointsOffset;
            SelectedSampleIndex = (int)((samplesWidth - Width + location.X) / PointsOffset);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && location.Y < (Height - TitleHeight))
            {
                _isSelecting = true;
                OnClick(ref location);
                StartMouseCapture();
                return true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_isSelecting)
            {
                OnClick(ref location);
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isSelecting)
            {
                _isSelecting = false;
                EndMouseCapture();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            _isSelecting = false;
        }
    }
}
