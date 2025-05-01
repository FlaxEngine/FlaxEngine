// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Single row control for <see cref="Table"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    public class Row : Control
    {
        private Table _table;

        /// <summary>
        /// Gets the parent table that owns this row.
        /// </summary>
        public Table Table => _table;

        /// <summary>
        /// Gets or sets the cell values.
        /// </summary>
        public object[] Values { get; set; }

        /// <summary>
        /// Gets or sets the cell background colors. Null if unused, transparent values are ignored.
        /// </summary>
        public Color[] BackgroundColors { get; set; }

        /// <summary>
        /// Gets or sets the row depth level.
        /// </summary>
        public int Depth { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="Row"/> class.
        /// </summary>
        /// <param name="height">The height.</param>
        public Row(float height = 16)
        : base(0, 0, 100, height)
        {
            Depth = -1;

            var fontHeight = Style.Current.FontMedium.Height;
            if (Height < fontHeight)
                Height = fontHeight + 4;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var style = Style.Current;

            if (IsMouseOver)
            {
                Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), style.BackgroundHighlighted * 0.7f);
            }

            if (Values != null && _table?.Columns != null)
            {
                float x = 0;
                int end = Mathf.Min(Values.Length, _table.Columns.Length);
                var backgroundColors = BackgroundColors;
                for (int i = 0; i < end; i++)
                {
                    var column = _table.Columns[i];
                    var value = Values[i];
                    var width = _table.GetColumnWidth(i);

                    string text;
                    if (value == null)
                        text = string.Empty;
                    else if (column.FormatValue != null)
                        text = column.FormatValue(value);
                    else
                        text = value.ToString();

                    var rect = new Rectangle(x, 0, width, Height);
                    rect.Expand(-4);
                    float leftDepthMargin = 0;

                    if (column.UseExpandCollapseMode)
                    {
                        float arrowSize = 12.0f;
                        leftDepthMargin = arrowSize * (Depth + 1);

                        // Check if has any child events (next row should be it)
                        int nextIndex = IndexInParent + 1;
                        if (nextIndex < _table.ChildrenCount)
                        {
                            if (_table.Children[nextIndex] is Row row && row.Depth == Depth + 1)
                            {
                                // Tree node arrow                  
                                var arrowRect = new Rectangle(x + leftDepthMargin - arrowSize, (Height - arrowSize) * 0.5f, arrowSize, arrowSize);
                                Render2D.DrawSprite(row.Visible ? style.ArrowDown : style.ArrowRight, arrowRect, IsMouseOver ? style.Foreground : style.ForegroundGrey);
                            }
                        }
                    }

                    rect.X += leftDepthMargin;
                    rect.Width -= leftDepthMargin;

                    Render2D.PushClip(rect);
                    if (backgroundColors != null && backgroundColors[i].A > 0)
                        Render2D.FillRectangle(rect, backgroundColors[i]);
                    Render2D.DrawText(style.FontMedium, text, rect, style.Foreground, column.CellAlignment, TextAlignment.Center);
                    Render2D.PopClip();

                    x += width;
                }
            }
        }

        /// <inheritdoc />
        protected override void OnParentChangedInternal()
        {
            base.OnParentChangedInternal();

            _table = Parent as Table;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && Values != null && _table?.Columns != null)
            {
                float x = 0;
                int end = Mathf.Min(Values.Length, _table.Columns.Length);
                for (int i = 0; i < end; i++)
                {
                    var column = _table.Columns[i];
                    var width = _table.GetColumnWidth(i);

                    if (column.UseExpandCollapseMode)
                    {
                        float arrowSize = 12.0f;
                        float leftDepthMargin = arrowSize * (Depth + 1);
                        int nextIndex = IndexInParent + 1;
                        if (nextIndex < _table.ChildrenCount)
                        {
                            if (_table.Children[nextIndex] is Row row && row.Depth == Depth + 1)
                            {
                                var arrowRect = new Rectangle(x + leftDepthMargin - arrowSize, (Height - arrowSize) * 0.5f, arrowSize, arrowSize);

                                if (arrowRect.Contains(location))
                                {
                                    // Collapse/expand
                                    SetSubRowsVisible(!GetSubRowsVisible());
                                    return true;
                                }
                            }
                        }
                    }

                    x += width;
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        protected override void OnVisibleChanged()
        {
            base.OnVisibleChanged();

            // Hide child events (only if using depth mode)
            if (HasParent && Depth != -1 && !Visible)
            {
                SetSubRowsVisible(Visible);
            }
        }

        private bool GetSubRowsVisible()
        {
            for (int i = Parent.GetChildIndex(this) + 1; i < Parent.ChildrenCount; i++)
            {
                if (Parent.Children[i] is Row child)
                {
                    if (child.Depth == Depth + 1)
                        return child.Visible;
                    if (child.Depth <= Depth)
                        break;
                }
            }

            return true;
        }

        private void SetSubRowsVisible(bool visible)
        {
            for (int i = Parent.GetChildIndex(this) + 1; i < Parent.ChildrenCount; i++)
            {
                if (Parent.Children[i] is Row child)
                {
                    if (child.Depth == Depth + 1)
                        child.Visible = visible;
                    else if (child.Depth <= Depth)
                        break;
                }
            }
        }
    }

    /// <summary>
    /// The table row that contains events for mouse interaction.
    /// </summary>
    [HideInEditor]
    public class ClickableRow : Row
    {
        private bool _leftClick;
        private bool _isRightDown;

        /// <summary>
        /// The double click event.
        /// </summary>
        public Action DoubleClick;

        /// <summary>
        /// The left mouse button click event.
        /// </summary>
        public Action LeftClick;

        /// <summary>
        /// The right mouse button click event.
        /// </summary>
        public Action RightClick;

        /// <summary>
        /// The double click event.
        /// </summary>
        public Action<ClickableRow> RowDoubleClick;

        /// <summary>
        /// The left mouse button click event.
        /// </summary>
        public Action<ClickableRow> RowLeftClick;

        /// <summary>
        /// The right mouse button click event.
        /// </summary>
        public Action<ClickableRow> RowRightClick;

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            DoubleClick?.Invoke();
            RowDoubleClick?.Invoke(this);

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
                _leftClick = true;
            else if (button == MouseButton.Right)
                _isRightDown = true;

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _leftClick)
            {
                _leftClick = false;
                LeftClick?.Invoke();
                RowLeftClick?.Invoke(this);
            }
            else if (button == MouseButton.Right && _isRightDown)
            {
                _isRightDown = false;
                RightClick?.Invoke();
                RowRightClick?.Invoke(this);
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _leftClick = false;
            _isRightDown = false;

            base.OnMouseLeave();
        }
    }
}
