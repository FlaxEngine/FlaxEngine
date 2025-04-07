// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.GUI
{
    /// <summary>
    /// <see cref="CustomEditor"/> properties list control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.PanelWithMargins" />
    [HideInEditor]
    public class PropertiesList : PanelWithMargins
    {
        // TODO: sync splitter for whole presenter

        /// <summary>
        /// The splitter size (in pixels).
        /// </summary>
        public const int SplitterSize = 2;

        /// <summary>
        /// The splitter margin (in pixels).
        /// </summary>
        public const int SplitterMargin = 4;

        private const int SplitterSizeHalf = SplitterSize / 2;

        private PropertiesListElement _element;
        private float _splitterValue;
        private Rectangle _splitterRect;
        private bool _splitterClicked, _mouseOverSplitter;
        private bool _cursorChanged;

        /// <summary>
        /// Gets or sets the splitter value (always in range [0; 1]).
        /// </summary>
        /// <value>
        /// The splitter value (always in range [0; 1]).
        /// </value>
        public float SplitterValue
        {
            get => _splitterValue;
            set
            {
                value = Mathf.Clamp(value, 0.05f, 0.95f);
                if (!Mathf.NearEqual(_splitterValue, value))
                {
                    _splitterValue = value;
                    UpdateSplitRect();
                    PerformLayout(true);
                }
            }
        }

        /// <summary>
        /// Gets the properties list element. It's a parent object for this control.
        /// </summary>
        public PropertiesListElement Element => _element;

        /// <summary>
        /// Initializes a new instance of the <see cref="PropertiesList"/> class.
        /// </summary>
        /// <param name="element">The element.</param>
        public PropertiesList(PropertiesListElement element)
        {
            _element = element;
            _splitterValue = 0.4f;
            BottomMargin = TopMargin = RightMargin = SplitterMargin;
            UpdateSplitRect();
        }

        private void UpdateSplitRect()
        {
            _splitterRect = new Rectangle(Mathf.Clamp(_splitterValue * Width - SplitterSizeHalf, 0.0f, Width), 0, SplitterSize, Height);
            LeftMargin = _splitterValue * Width + SplitterMargin;
        }

        private void StartTracking()
        {
            // Start move
            _splitterClicked = true;

            // Start capturing mouse
            StartMouseCapture();
        }

        private void EndTracking()
        {
            if (_splitterClicked)
            {
                // Clear flag
                _splitterClicked = false;

                // End capturing mouse
                EndMouseCapture();
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var style = Style.Current;

            // Draw splitter
            Render2D.FillRectangle(_splitterRect, _splitterClicked ? style.BackgroundSelected : _mouseOverSplitter ? style.BackgroundHighlighted : style.Background * 0.8f);
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
            _mouseOverSplitter = _splitterRect.Contains(location);

            if (_splitterClicked)
            {
                SplitterValue = location.X / Width;
                Cursor = CursorType.SizeWE;
                _cursorChanged = true;
            }
            else if (_mouseOverSplitter)
            {
                Cursor = CursorType.SizeWE;
                _cursorChanged = true;
            }
            else if (_cursorChanged)
            {
                Cursor = CursorType.Default;
                _cursorChanged = false;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                if (_splitterRect.Contains(location))
                {
                    // Start moving splitter
                    StartTracking();
                    return false;
                }
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (_splitterClicked)
            {
                EndTracking();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Clear flag
            _mouseOverSplitter = false;

            if (_cursorChanged)
            {
                Cursor = CursorType.Default;
                _cursorChanged = false;
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            // Clear flag
            _splitterClicked = false;
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            // Refresh
            UpdateSplitRect();
            PerformLayout(true);
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            // Pre-set width of all controls
            float w = Width - _margin.Width;
            for (int i = 0; i < _children.Count; i++)
            {
                Control c = _children[i];
                if (!(c is PropertyNameLabel))
                {
                    c.Width = w;
                }
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            // Sort controls from up to down into two columns: one for labels and one for the rest of the stuff

            float y = _margin.Top;
            float w = Width - _margin.Width;
            for (int i = 0; i < _children.Count; i++)
            {
                Control c = _children[i];
                if (!(c is PropertyNameLabel))
                {
                    var h = c.Height;
                    c.Bounds = new Rectangle(_margin.Left, y + _spacing, w, h);
                    if (c.Visible)
                        y = c.Bottom;
                }
            }
            y += _margin.Bottom;

            float namesWidth = _splitterValue * Width;
            int count = _element.Labels.Count;
            float[] yStarts = new float[count + 1];
            for (int i = 0; i < count; i++)
            {
                var label = _element.Labels[i];
                var container = label.FirstChildControlContainer ?? this;

                if (label.FirstChildControlIndex < 0)
                    yStarts[i] = 0;
                else if (container.ChildrenCount <= label.FirstChildControlIndex)
                    yStarts[i] = y;
                else if (label.FirstChildControlContainer != null)
                {
                    var firstChild = label.FirstChildControlContainer.Children[label.FirstChildControlIndex];
                    yStarts[i] = firstChild.PointToParent(this, Float2.Zero).Y;
                    if (i == count - 1)
                        yStarts[i + 1] = firstChild.Parent.Bottom;
                }
                else
                {
                    var firstChild = _children[label.FirstChildControlIndex];
                    yStarts[i] = firstChild.Top;
                    if (i == count - 1)
                        yStarts[i + 1] = firstChild.Bottom;
                }
                   
            }
            for (int i = 0; i < count; i++)
            {
                var label = _element.Labels[i];

                var rect = new Rectangle(0, yStarts[i] + 1, namesWidth, yStarts[i + 1] - yStarts[i] - 2);
                //label.Parent = this;
                label.Bounds = rect;
            }

            if (_autoSize)
                Height = y;
        }
    }
}
