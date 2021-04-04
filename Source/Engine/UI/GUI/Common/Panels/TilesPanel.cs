// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Panel that arranges child controls like tiles.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class TilesPanel : ContainerControl
    {
        private Vector2 _tileSize = new Vector2(64);
        private bool _autoResize = false;

        /// <summary>
        /// Gets or sets the size of the tile.
        /// </summary>
        [EditorOrder(0), Limit(0.01f), Tooltip("The size of the single tile.")]
        public Vector2 TileSize
        {
            get => _tileSize;
            set
            {
                if (value.MinValue <= 0.0f)
                    throw new ArgumentException("Tiles cannot have negative size.");

                if (!Vector2.Equals(ref _tileSize, ref value))
                {
                    _tileSize = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether perform automatic resize after tiles arrange.
        /// </summary>
        [EditorOrder(10), Tooltip("If checked, the automatic control resize will be performed after tiles arrange,")]
        public bool AutoResize
        {
            get => _autoResize;
            set
            {
                if (_autoResize != value)
                {
                    _autoResize = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TilesPanel"/> class.
        /// </summary>
        public TilesPanel()
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            // Calculate items size
            float width = Width;
            /*
            maybe use auto fit mode?
            float defaultItemsWidth = _tileSize.X;
            int itemsToFit = Mathf.FloorToInt(width / defaultItemsWidth);
            float itemsWidth = width / Mathf.Max(itemsToFit, 1);
            float itemsHeight = itemsWidth / defaultItemsWidth * _tileSize.Y;
            */
            float itemsWidth = _tileSize.X;
            float itemsHeight = _tileSize.Y;

            // Arrange controls
            float x = 0, y = 0;
            for (int i = 0; i < _children.Count; i++)
            {
                var c = _children[i];

                c.Bounds = new Rectangle(x, y, itemsWidth, itemsHeight);

                x += itemsWidth;
                if (x + itemsWidth > width)
                {
                    x = 0;
                    y += itemsHeight;
                }
            }
            if (x > 0)
                y += itemsHeight;

            if (_autoResize)
            {
                Height = y;
            }
        }
    }
}
