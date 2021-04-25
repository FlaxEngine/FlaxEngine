// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Editor control that allows user to select a platform icon.
    /// </summary>
    public class PlatformSelector : TilesPanel
    {
        /// <summary>
        /// Simple button just for platform.
        /// </summary>
        public class PlatformImage : Image
        {
            /// <summary>
            /// Gets or sets the color used when mouse is over the image.
            /// </summary>
            [EditorDisplay("Style"), EditorOrder(2000)]
            public Color MouseOverColor { get; set; } = Color.Gray;

            /// <summary>
            /// Gets or sets the color used.
            /// </summary>
            [EditorDisplay("Style"), EditorOrder(2000)]
            public Color NormalColor { get; set; } = Color.White;

            /// <summary>
            /// Changing color on Mouse Enter.
            /// </summary>
            public override void Draw()
            {
                Color = IsMouseOver ? MouseOverColor : NormalColor;
                base.Draw();
            }
        }

        private struct PlatformData
        {
            public PlatformType PlatformType;
            public SpriteHandle Icon;
            public string PlatformName;

            public PlatformData(PlatformType type, SpriteHandle icon, string name)
            {
                PlatformType = type;
                Icon = icon;
                PlatformName = name;
            }
        }

        private Color _mouseOverColor;
        private Color _selectedColor;
        private Color _defaultColor;
        private PlatformType _selected;

        /// <summary>
        /// Gets or sets the selected platform.
        /// </summary>
        public PlatformType Selected
        {
            get => _selected;
            set
            {
                if (_selected != value)
                {
                    bool isValid = false;
                    for (int i = 0; i < _children.Count; i++)
                    {
                        if (_children[i] is PlatformImage img)
                        {
                            if ((PlatformType)img.Tag == value)
                            {
                                isValid = true;
                                img.NormalColor = _selectedColor;
                                img.MouseOverColor = _selectedColor;
                            }
                            else
                            {
                                img.NormalColor = _defaultColor;
                                img.MouseOverColor = _mouseOverColor;
                            }
                        }
                    }

                    if (isValid)
                    {
                        _selected = value;
                        SelectedChanged?.Invoke(_selected);
                    }
                }
            }
        }

        /// <summary>
        /// Occurs when selected platform gets changed.
        /// </summary>
        public event Action<PlatformType> SelectedChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="PlatformSelector"/> class.
        /// </summary>
        public PlatformSelector()
        {
            var style = Style.Current;
            var icons = Editor.Instance.Icons;
            var platforms = new[]
            {
                new PlatformData(PlatformType.Windows, icons.Windows, "Windows"),
                new PlatformData(PlatformType.XboxOne, icons.XboxOne, "Xbox One"),
                new PlatformData(PlatformType.UWP, icons.WindowsStore, "Windows Store"),
                new PlatformData(PlatformType.Linux, icons.Linux, "Linux"),
                new PlatformData(PlatformType.PS4, icons.PS4, "PlayStation 4"),
                new PlatformData(PlatformType.XboxScarlett, icons.XboxSeriesX, "Xbox Scarlett"),
                new PlatformData(PlatformType.Android, icons.Android, "Android"),
            };

            const float IconSize = 48.0f;
            TileSize = new Vector2(IconSize);
            AutoResize = true;
            Offsets = new Margin(0, 0, 0, IconSize);

            _mouseOverColor = style.Foreground;
            _selectedColor = style.Foreground;
            _defaultColor = style.ForegroundGrey;

            for (int i = 0; i < platforms.Length; i++)
            {
                var tile = new PlatformImage
                {
                    Brush = new SpriteBrush(platforms[i].Icon),
                    MouseOverColor = _mouseOverColor,
                    NormalColor = _defaultColor,
                    Tag = platforms[i].PlatformType,
                    TooltipText = platforms[i].PlatformName,
                    Parent = this,
                };
                tile.Clicked += OnTileClicked;
            }

            // Select the first platform
            _selected = platforms[0].PlatformType;
            ((PlatformImage)Children[0]).NormalColor = _selectedColor;
            ((PlatformImage)Children[0]).MouseOverColor = _selectedColor;
        }

        private void OnTileClicked(Image image, MouseButton mouseButton)
        {
            if (mouseButton == MouseButton.Left)
            {
                Selected = (PlatformType)image.Tag;
            }
        }
    }
}
