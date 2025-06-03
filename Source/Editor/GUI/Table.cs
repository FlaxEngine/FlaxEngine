// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Table control with columns and rows.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class Table : ContainerControl
    {
        private float _headerHeight = 20;
        private ColumnDefinition[] _columns;
        private float[] _splits;
        private int _movingSplit = -1;
        private Float2 _mousePos;

        /// <summary>
        /// Gets or sets the height of the table column headers.
        /// </summary>
        public float HeaderHeight
        {
            get => _headerHeight;
            set
            {
                value = Mathf.Max(value, 1);
                if (!Mathf.NearEqual(value, _headerHeight))
                {
                    _headerHeight = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the columns description.
        /// </summary>
        public ColumnDefinition[] Columns
        {
            get => _columns;
            set
            {
                _columns = value;

                if (_columns == null)
                {
                    // Unlink splits
                    _splits = null;
                }
                else
                {
                    // Set the default values for the splits
                    _splits = new float[_columns.Length];
                    float split = 1.0f / _columns.Length;
                    for (int i = 0; i < _splits.Length; i++)
                        _splits[i] = split;
                }

                PerformLayout();
            }
        }

        /// <summary>
        /// The column split values. Specified per column as normalized value to range [0;1]. Actual column width is calculated by multiplication of split value and table width.
        /// </summary>
        public float[] Splits
        {
            get => _splits;
            set
            {
                if (_columns == null)
                    return;
                if (value == null || value.Length != _columns.Length)
                    throw new ArgumentException();

                _splits = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Table"/> class.
        /// </summary>
        public Table()
        {
        }

        /// <summary>
        /// Gets the actual column width (in pixels).
        /// </summary>
        /// <param name="columnIndex">Zero-based index of the column.</param>
        /// <returns>The column width in pixels.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float GetColumnWidth(int columnIndex)
        {
            return _splits[columnIndex] * Width;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (_columns != null && _splits != null)
            {
                Rectangle rect = new Rectangle(0, 0, 0, _headerHeight);
                for (int i = 0; i < _columns.Length; i++)
                {
                    rect.Width = GetColumnWidth(i);
                    DrawColumn(ref rect, i);
                    rect.X += rect.Width;
                }
            }
        }

        /// <summary>
        /// Draws the column.
        /// </summary>
        /// <param name="rect">The header area rectangle.</param>
        /// <param name="columnIndex">The zero-based index of the column.</param>
        protected virtual void DrawColumn(ref Rectangle rect, int columnIndex)
        {
            var column = _columns[columnIndex];

            Render2D.FillRectangle(rect, column.TitleBackgroundColor);

            var style = Style.Current;
            var font = column.TitleFont ?? style.FontMedium;
            var textRect = rect;
            column.TitleMargin.ShrinkRectangle(ref textRect);
            Render2D.DrawText(font, column.Title, textRect, column.TitleColor, column.TitleAlignment, TextAlignment.Center);

            if (columnIndex < _columns.Length - 1)
            {
                var splitRect = new Rectangle(rect.Right - 2, 2, 4, rect.Height - 4);
                Render2D.FillRectangle(splitRect, _movingSplit == columnIndex || splitRect.Contains(_mousePos) ? style.BorderNormal : style.Background * 0.9f);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                if (_columns != null && _splits != null)
                {
                    Rectangle rect = new Rectangle(0, 0, 0, _headerHeight);
                    for (int i = 0; i < _columns.Length - 1; i++)
                    {
                        rect.Width = GetColumnWidth(i);

                        var splitRect = new Rectangle(rect.Right - 2, 2, 4, rect.Height - 4);
                        if (splitRect.Contains(location))
                        {
                            // Start moving splitter
                            _movingSplit = i;
                            StartMouseCapture();
                            Cursor = CursorType.SizeWE;
                            break;
                        }

                        rect.X += rect.Width;
                    }
                }
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            _mousePos = location;

            if (_movingSplit != -1)
            {
                int nextSplit = _movingSplit + 1;
                float width = Width;
                float leftPos = 0;
                for (int i = 0; i < _movingSplit; i++)
                    leftPos += _splits[i];
                leftPos *= width;

                float movingLength = GetColumnWidth(_movingSplit) + GetColumnWidth(nextSplit);
                float splitsSum = _splits[_movingSplit] + _splits[nextSplit];

                float leftSplit = splitsSum * Mathf.Saturate((location.X - leftPos) / movingLength);
                _splits[_movingSplit] = _columns[_movingSplit].ClampColumnSize(leftSplit, width);
                float rightSplit = splitsSum - _splits[_movingSplit];
                _splits[nextSplit] = _columns[nextSplit].ClampColumnSize(rightSplit, width);

                PerformLayout();
            }
            else
            {
                if (_columns != null && _splits != null)
                {
                    Rectangle rect = new Rectangle(0, 0, 0, _headerHeight);
                    for (int i = 0; i < _columns.Length - 1; i++)
                    {
                        rect.Width = GetColumnWidth(i);

                        var splitRect = new Rectangle(rect.Right - 2, 2, 4, rect.Height - 4);
                        if (splitRect.Contains(location))
                        {
                            // Start moving splitter
                            Cursor = CursorType.SizeWE;
                            break;
                        }
                        else
                        {
                            Cursor = CursorType.Default;
                        }

                        rect.X += rect.Width;
                    }
                }
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _movingSplit != -1)
            {
                _movingSplit = -1;
                EndMouseCapture();
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            _movingSplit = -1;
            Cursor = CursorType.Default;
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            base.PerformLayoutAfterChildren();

            // Arrange rows
            float y = _headerHeight;
            for (int i = 0; i < Children.Count; i++)
            {
                var c = Children[i];
                if (c.Visible)
                {
                    var bounds = c.Bounds;
                    bounds.Width = Width;
                    bounds.Y = y;
                    c.Bounds = bounds;
                    y += bounds.Height + 1;
                }
            }

            Height = y;
        }
    }
}
