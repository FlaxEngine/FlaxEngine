// Copyright (c) Wojciech Figat. All rights reserved.

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
                        if (_children[i] is Image img)
                        {
                            if ((PlatformType)img.Tag == value)
                            {
                                isValid = true;
                                img.Color = _selectedColor;
                                img.MouseOverColor = _selectedColor;
                            }
                            else
                            {
                                img.Color = _defaultColor;
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
                new PlatformData(PlatformType.Windows, icons.WindowsIcon128, "Windows"),
                new PlatformData(PlatformType.XboxOne, icons.XBoxOne128, "Xbox One"),
                new PlatformData(PlatformType.UWP, icons.UWPStore128, "Windows Store"),
                new PlatformData(PlatformType.Linux, icons.LinuxIcon128, "Linux"),
                new PlatformData(PlatformType.PS4, icons.PS4Icon128, "PlayStation 4"),
                new PlatformData(PlatformType.XboxScarlett, icons.XBoxScarletIcon128, "Xbox Scarlett"),
                new PlatformData(PlatformType.Android, icons.AndroidIcon128, "Android"),
                new PlatformData(PlatformType.Switch, icons.SwitchIcon128, "Switch"),
                new PlatformData(PlatformType.PS5, icons.PS5Icon128, "PlayStation 5"),
                new PlatformData(PlatformType.Mac, icons.MacOSIcon128, "macOS"),
                new PlatformData(PlatformType.iOS, icons.IOSIcon128, "iOS"),
            };

            const float IconSize = 64.0f;
            TileSize = new Float2(IconSize);
            AutoResize = true;
            Offsets = new Margin(0, 0, 0, IconSize);

            // Ignoring style on purpose (style would make sense if the icons were white, but they are colored)
            _mouseOverColor = new Color(0.8f, 0.8f, 0.8f, 1f);
            _selectedColor = Color.White;
            _defaultColor = new Color(0.7f, 0.7f, 0.7f, 0.5f);

            for (int i = 0; i < platforms.Length; i++)
            {
                var tile = new Image
                {
                    Brush = new SpriteBrush(platforms[i].Icon),
                    MouseOverColor = _mouseOverColor,
                    Color = _defaultColor,
                    Tag = platforms[i].PlatformType,
                    TooltipText = platforms[i].PlatformName,
                    Parent = this,
                };
                tile.Clicked += OnTileClicked;
            }

            // Select the first platform
            _selected = platforms[0].PlatformType;
            ((Image)Children[0]).Color = _selectedColor;
            ((Image)Children[0]).MouseOverColor = _selectedColor;
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
