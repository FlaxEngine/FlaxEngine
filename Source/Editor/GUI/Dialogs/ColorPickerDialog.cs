// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Input;
using FlaxEditor.GUI.Tabs;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using System.Collections.Generic;
using System.Linq;

namespace FlaxEditor.GUI.Dialogs
{
    /// <summary>
    /// The base interface for the color picker dialogs.
    /// </summary>
    public interface IColorPickerDialog
    {
        /// <summary>
        /// Closes the picker (similar to value editing cancel).
        /// </summary>
        void ClosePicker();
    }

    /// <summary>
    /// Color picking dialog.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Dialogs.Dialog" />
    public class ColorPickerDialog : Dialog, IColorPickerDialog
    {
        private const float ChannelsMargin = 4.0f;
        private const float SavedColorButtonWidth = 20.0f;
        private const float SavedColorButtonHeight = 20.0f;
        private const float ColorPreviewWidth = 118.0f;
        private const float OldNewColorPreviewDisplayRatio = 0.35f;
        private const float RevertOldColorSpriteSize = 18.0f;
        private const float ColorWheelSize = 180.0f;
        private const float SaturationAlphaSlidersThickness = 20.0f;
        private const float HSVRGBFloatBoxesWidht = 100;
        private const float GridScale = 4f;
        private const string AddSavedColorButtonTooltip = "Save current color.";
        private const string SavedColorButtonTooltip = "Saved color";
        private const string ScaleParamName = "Scale";

        private const int maxSavedColorsAmount = 20; // Will begin new row when half of this value is reached.

        private const float SmallMargin = 6.0f;
        private const float MediumMargin = 15.0f;
        private const float LargeMargin = 25.0f;
      
        private Margin _pickerMargin = new Margin(SmallMargin, SmallMargin, SmallMargin, SmallMargin);
        private Margin _textBoxBlockMargin = new Margin(LargeMargin + 1.5f, SmallMargin + 1.5f, SmallMargin + 1.5f, SmallMargin + 1.5f);
        private Margin _hsvRgbFloatBoxBlockMargin = new Margin(MediumMargin, MediumMargin, SmallMargin, SmallMargin);

        private Color _eyedropperStartColor;
        private Color _initialValue;
        private Color _value;
        private bool _disableEvents;
        private bool _useDynamicEditing;
        private bool _isEyedropperActive;
        private ColorValueBox.ColorPickerEvent _onChanged;
        private ColorValueBox.ColorPickerClosedEvent _onClosed;

        private ColorSelectorWithSliders _cSelector;
        private Rectangle oldColorRect;
        private Rectangle newColorRect;
        private Tabs.Tabs _chsvRGBTabs;
        private Tab _cRGBTab;
        private Panel rgbPanel;
        private FloatValueBox _cRed;
        private FloatValueBox _cGreen;
        private FloatValueBox _cBlue;
        private Tab _cHSVTab;
        private Panel hsvPanel;
        private FloatValueBox _cHue;
        private FloatValueBox _cSaturation;
        private FloatValueBox _cValue;
        private FloatValueBox _cAlpha;
        private TextBox _cHex;
        private Button _cEyedropper;
        private MaterialBase _checkerMaterial;

        private List<Color> _savedColors = new List<Color>();
        private List<SavedColorButton> _savedColorButtons = new List<SavedColorButton>();

        /// <summary>
        /// Gets the selected color.
        /// </summary>
        public Color SelectedColor
        {
            get => _value;
            private set
            {
                if (_disableEvents || value == _value)
                    return;

                _disableEvents = true;
                _value = value;
                var clamped = Color.Min(Color.White, value);

                // Selector
                _cSelector.Color = _value;

                // RGBA
                _cRed.Value = _value.R;
                _cGreen.Value = _value.G;
                _cBlue.Value = _value.B;
                _cAlpha.Value = _value.A;

                // HSV
                var hsv = _value.ToHSV();
                _cHue.Value = hsv.X;
                _cSaturation.Value = hsv.Y * 100.0f;
                _cValue.Value = hsv.Z * 100.0f;

                // Hex
                _cHex.Text = clamped.ToHexString();

                // Dynamic value
                if (_useDynamicEditing)
                    _onChanged?.Invoke(_value, true);

                _disableEvents = false;

                UpdateAddSavedColorsButton();
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ColorPickerDialog"/> class.
        /// </summary>
        /// <param name="initialValue">The initial value.</param>
        /// <param name="colorChanged">The color changed event.</param>
        /// <param name="pickerClosed">The close event.</param>
        /// <param name="useDynamicEditing">True if allow dynamic value editing (slider-like usage), otherwise will fire color change event only on editing end.</param>
        public ColorPickerDialog(Color initialValue, ColorValueBox.ColorPickerEvent colorChanged, ColorValueBox.ColorPickerClosedEvent pickerClosed, bool useDynamicEditing)
        : base("Pick a color!")
        {
            _initialValue = initialValue;
            _useDynamicEditing = useDynamicEditing;
            _value = Color.Transparent;
            _onChanged = colorChanged;
            _onClosed = pickerClosed;

            // Load alpha grid material
            _checkerMaterial = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.ColorAlphaBackgroundGrid);
            _checkerMaterial = _checkerMaterial.CreateVirtualInstance();
            _checkerMaterial.SetParameterValue(ScaleParamName, GridScale);

            // Get saved colors if they exist
            if (Editor.Instance.ProjectCache.TryGetCustomData("ColorPickerSavedColors", out string savedColors))
                _savedColors = JsonSerializer.Deserialize<List<Color>>(savedColors);

            // Selector
            _cSelector = new ColorSelectorWithSliders(ColorWheelSize, SaturationAlphaSlidersThickness)
            {
                Location = new Float2(_pickerMargin.Left * 2, _pickerMargin.Top * 2),
                Parent = this
            };
            _cSelector.ColorChanged += x => SelectedColor = x;

            // Tabs tab container
            const float TabsSizeY = 20;
            _chsvRGBTabs = new Tabs.Tabs
            {
                Location = new Float2(_cSelector.Right + 15 + LargeMargin, _pickerMargin.Top + _pickerMargin.Top),
                TabsTextHorizontalAlignment = TextAlignment.Center,
                Width = HSVRGBFloatBoxesWidht + _hsvRgbFloatBoxBlockMargin.Left * 2,
                Height = (FloatValueBox.DefaultHeight + ChannelsMargin) * 4 + ChannelsMargin,
                Parent = this,
            };
            _chsvRGBTabs.TabsSize = new Float2(_chsvRGBTabs.Width * 0.5f, TabsSizeY);

            // RGB Tab
            _cRGBTab = _chsvRGBTabs.AddTab(new Tab("RGB"));
            rgbPanel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _cRGBTab
            };

            // HSV Tab
            _cHSVTab = _chsvRGBTabs.AddTab(new Tab("HSV"));
            hsvPanel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _cHSVTab
            };

            _chsvRGBTabs.SelectedTabChanged += SelectedTabChanged;

            // Red
            _cRed = new FloatValueBox(0, _hsvRgbFloatBoxBlockMargin.Left, _hsvRgbFloatBoxBlockMargin.Top, HSVRGBFloatBoxesWidht, 0, float.MaxValue, 0.001f)
            {
                Parent = rgbPanel
            };
            _cRed.ValueChanged += OnRGBAChanged;

            // Green
            _cGreen = new FloatValueBox(0, _hsvRgbFloatBoxBlockMargin.Left, _cRed.Bottom + ChannelsMargin, _cRed.Width, 0, float.MaxValue, 0.001f)
            {
                Parent = rgbPanel
            };
            _cGreen.ValueChanged += OnRGBAChanged;

            // Blue
            _cBlue = new FloatValueBox(0, _hsvRgbFloatBoxBlockMargin.Left, _cGreen.Bottom + ChannelsMargin, _cRed.Width, 0, float.MaxValue, 0.001f)
            {
                Parent = rgbPanel
            };
            _cBlue.ValueChanged += OnRGBAChanged;

            // Hue
            _cHue = new FloatValueBox(0, _hsvRgbFloatBoxBlockMargin.Left, _hsvRgbFloatBoxBlockMargin.Top, HSVRGBFloatBoxesWidht)
            {
                Parent = hsvPanel
            };
            _cHue.ValueChanged += OnHSVChanged;

            // Saturation
            _cSaturation = new FloatValueBox(0, _hsvRgbFloatBoxBlockMargin.Left, _cHue.Bottom + ChannelsMargin, _cHue.Width, 0, 100.0f, 0.1f)
            {
                Parent = hsvPanel
            };
            _cSaturation.ValueChanged += OnHSVChanged;

            // Value
            _cValue = new FloatValueBox(0, _hsvRgbFloatBoxBlockMargin.Left, _cSaturation.Bottom + ChannelsMargin, _cHue.Width, 0, float.MaxValue, 0.1f)
            {
                Parent = hsvPanel
            };
            _cValue.ValueChanged += OnHSVChanged;

            // Alpha
            float alphaXPos = hsvPanel.PointToWindow(_cBlue.BottomLeft).X;
            _cAlpha = new FloatValueBox(0, alphaXPos, _chsvRGBTabs.Bottom + MediumMargin, _cRed.Width, 0, float.MaxValue, 0.001f)
            {
                Parent = this
            };
            _cAlpha.ValueChanged += OnRGBAChanged;

            // Hex
            _cHex = new TextBox(false, _cAlpha.Location.X, _cAlpha.Bottom + ChannelsMargin * 2, _cRed.Width)
            {
                Parent = this
            };
            _cHex.EditEnd += OnHexChanged;

            // Set valid dialog size based on UI contents
            _dialogSize = Size = new Float2(_chsvRGBTabs.Right + _pickerMargin.Left + _pickerMargin.Left, _cSelector.Bottom + SmallMargin + SavedColorButtonHeight * 3 + _pickerMargin.Top * 3);

            CreateSavedSaveColorButtons();
            UpdateAddSavedColorsButton();

            // Old and new color preview rectangles
            float previewYPosition = _cSelector.Bottom + LargeMargin;

            float oldPreviewHeight = SavedColorButtonHeight * 2 + SmallMargin; // Make the rect the same height as saved colors (when there are two rows)
            oldColorRect = new Rectangle(_cSelector.Right + 15 + LargeMargin, previewYPosition, ColorPreviewWidth * OldNewColorPreviewDisplayRatio, oldPreviewHeight);
            newColorRect = new Rectangle(oldColorRect.Right, previewYPosition, ColorPreviewWidth * (1 - OldNewColorPreviewDisplayRatio), oldColorRect.Height);

            // Eyedropper button
            var style = Style.Current;
            _cEyedropper = new Button(_cSelector.BottomLeft.X, _cSelector.BottomLeft.Y)
            {
                TooltipText = "Eyedropper tool to pick a color directly from the screen.",
                BackgroundBrush = new SpriteBrush(Editor.Instance.Icons.Search32),
                BackgroundColor = style.Foreground,
                BackgroundColorHighlighted = style.Foreground.RGBMultiplied(0.9f),
                BorderColor = Color.Transparent,
                BorderColorHighlighted = style.BorderSelected,
                Parent = this,
            };
            _cEyedropper.Clicked += OnEyedropStart;
            _cEyedropper.Width = _cEyedropper.Height;
            _cEyedropper.Location = new Float2(_cEyedropper.Location.X, _cEyedropper.Location.Y - _cEyedropper.Height);

            // Set initial color
            SelectedColor = initialValue;
        }

        private void SelectedTabChanged(Tabs.Tabs tabs)
        {
            switch (tabs.SelectedTabIndex)
            {
                // RGB
                case 0:
                    rgbPanel.Visible = true;
                    hsvPanel.Visible = false;
                    break;
                // HSV
                case 1:
                    rgbPanel.Visible = false;
                    hsvPanel.Visible = true;
                    break;
            }
        }

        private void OnSavedColorButtonClicked(Button button)
        {
            // "+" button clicked
            if (button.Tag == null)
            {             
                // Set color of button to current value;
                button.BackgroundColor = _value;
                button.BackgroundColorHighlighted = _value;
                button.BackgroundColorSelected = _value.RGBMultiplied(0.8f);
                button.Text = "";
                button.TooltipText = $"{SavedColorButtonTooltip} {_savedColors.Count + 1}.";
                button.Tag = _value;

                // Save new colors
                _savedColors.Add(_value);
                var savedColors = JsonSerializer.Serialize(_savedColors, typeof(List<Color>));
                Editor.Instance.ProjectCache.SetCustomData("ColorPickerSavedColors", savedColors);

                float savedColorsYPosition = _cSelector.Bottom + LargeMargin;

                // Create new + button
                if (_savedColorButtons.Count < maxSavedColorsAmount)
                    CreateSaveNewColorButton(savedColorsYPosition);
            }
            // Button with saved color clicked
            else
                SelectedColor = (Color)button.Tag;

            UpdateAddSavedColorsButton();
        }

        private void OnColorPicked(Color32 colorPicked)
        {
            if (_isEyedropperActive)
            {
                _isEyedropperActive = false;

                // Reset colors
                _cEyedropper.BackgroundColor = Style.Current.Foreground;
                _cEyedropper.BorderColor = Color.Transparent;

                SelectedColor = colorPicked;
                ScreenUtilities.PickColorDone -= OnColorPicked;
            }
        }

        private void OnEyedropStart()
        {
            _isEyedropperActive = true;
            _eyedropperStartColor = _value;

            // Provide some visual feedback that the eyedropper is active by changing button colors
            _cEyedropper.BackgroundColor = _cEyedropper.BackgroundColorSelected;
            _cEyedropper.BorderColor = _cEyedropper.BorderColorSelected;

            ScreenUtilities.PickColor();
            ScreenUtilities.PickColorDone += OnColorPicked;
        }

        private void OnRGBAChanged()
        {
            if (_disableEvents)
                return;

            SelectedColor = new Color(_cRed.Value, _cGreen.Value, _cBlue.Value, _cAlpha.Value);
        }

        private void OnHSVChanged()
        {
            if (_disableEvents)
                return;

            // Loop the hue float value box
            if (_cHue.Value < 0)
                _cHue.Value = 360.0f - Mathf.Abs(_cHue.Value);
            else if (_cHue.Value > 360)
                _cHue.Value = _cHue.Value - 360.0f;

            SelectedColor = Color.FromHSV(_cHue.Value, _cSaturation.Value / 100.0f, _cValue.Value / 100.0f, _cAlpha.Value);
        }

        private void OnHexChanged()
        {
            if (_disableEvents)
                return;

            if (Color.TryParseHex(_cHex.Text, out Color color))
                SelectedColor = color;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Update eye dropper tool
            if (_isEyedropperActive)
            {
                Float2 mousePosition = Platform.MousePosition;
                SelectedColor = ScreenUtilities.GetColorAt(mousePosition);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var textColor = style.Foreground;

            base.Draw();

            switch (_chsvRGBTabs.SelectedTabIndex)
            { 
                // RGB
                case 0:
                    Float2 rgbTextX = rgbPanel.PointToWindow(rgbPanel.Location);
                    var rgbRect = new Rectangle(rgbTextX.X, rgbPanel.PointToWindow(_cRed.Location).Y, 25, _cRed.Height);

                    Render2D.DrawText(style.FontMedium, "R", rgbRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    rgbRect.Location.Y = rgbPanel.PointToWindow(_cGreen.Location).Y;
                    Render2D.DrawText(style.FontMedium, "G", rgbRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    rgbRect.Location.Y = rgbPanel.PointToWindow(_cBlue.Location).Y;
                    Render2D.DrawText(style.FontMedium, "B", rgbRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    break;
                // HSV
                case 1:
                    // HSV letters (left)
                    Float2 hsvTextX = rgbPanel.PointToWindow(hsvPanel.Location);
                    var hsvLRect = new Rectangle(hsvTextX.X, hsvPanel.PointToWindow(_cHue.Location).Y, 25, _cHue.Height);

                    Render2D.DrawText(style.FontMedium, "H", hsvLRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    hsvLRect.Location.Y = hsvPanel.PointToWindow(_cSaturation.Location).Y;
                    Render2D.DrawText(style.FontMedium, "S", hsvLRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    hsvLRect.Location.Y = hsvPanel.PointToWindow(_cValue.Location).Y;
                    Render2D.DrawText(style.FontMedium, "V", hsvLRect, textColor, TextAlignment.Near, TextAlignment.Center);

                    // HSV math symbols (right)
                    Float2 hsvSymbolsX = rgbPanel.PointToWindow(rgbPanel.Location) + _cHue.Width + 20;
                    var hsvRRect = new Rectangle(hsvSymbolsX.X, hsvPanel.PointToWindow(_cHue.Location).Y, 25, _cHue.Height);

                    Render2D.DrawText(style.FontMedium, "°", hsvRRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    hsvRRect.Location.Y = hsvPanel.PointToWindow(_cSaturation.Location).Y;
                    Render2D.DrawText(style.FontMedium, "%", hsvRRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    hsvRRect.Location.Y = hsvPanel.PointToWindow(_cValue.Location).Y;
                    Render2D.DrawText(style.FontMedium, "%", hsvRRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    break;
            }

            // Alpha
            var alpha = new Rectangle(_cAlpha.Left - 15, _cAlpha.Y, 25, _cAlpha.Height);
            Render2D.DrawText(style.FontMedium, "A", alpha, textColor, TextAlignment.Near, TextAlignment.Center);

            // Hex
            var hex = new Rectangle(_cHex.Left - 26, _cHex.Y, 25, _cHex.Height);
            Render2D.DrawText(style.FontMedium, "Hex", hex, textColor, TextAlignment.Near, TextAlignment.Center);

            if (_initialValue.A < 1 || _value.A < 1)
            {
                // Draw checkerboard for background of color preview to help with transparency
                var checkerRect = new Rectangle(oldColorRect.Left, newColorRect.Y, ColorPreviewWidth, oldColorRect.Height);
                Render2D.DrawMaterial(_checkerMaterial, checkerRect);
            }

            // Old and new color preview
            Vector2 textOffset = new Vector2(0, -15);
            Render2D.DrawText(style.FontMedium, "Old", Color.White, oldColorRect.UpperLeft + textOffset);
            Render2D.DrawText(style.FontMedium, "New", Color.White, newColorRect.UpperLeft + textOffset);

            Render2D.FillRectangle(oldColorRect, _initialValue);
            Render2D.FillRectangle(newColorRect, _value);

            var oldhsv = _initialValue.ToHSV();
            var newhsv = _value.ToHSV();

            // Revert icon if color has changed
            if (_value != _initialValue)
            {
                Rectangle resetRect = new Rectangle(oldColorRect.Center - RevertOldColorSpriteSize * 0.5f, new Float2(RevertOldColorSpriteSize));
                Color resetColor = oldhsv.Z > 0.5f ? Color.Black : Color.White;
                Render2D.DrawSprite(Editor.Instance.Icons.Rotate32, resetRect, resetColor);
            }

            // Draw all outlines all as separate lines to prevent bleeding issues with separator lines

            // Draw Old outlines
            Color oldOutlineColor = oldhsv.X > 205 && oldhsv.Y > 0.65f ? Color.White : Color.Black;
            Render2D.DrawLine(oldColorRect.UpperLeft, oldColorRect.UpperRight, oldOutlineColor, 0.5f);
            Render2D.DrawLine(oldColorRect.UpperLeft, oldColorRect.BottomLeft, oldOutlineColor, 0.5f);
            Render2D.DrawLine(oldColorRect.BottomLeft, oldColorRect.BottomRight, oldOutlineColor, 0.5f);
            // Draw New outlines
            Color newOutlineColor = newhsv.X > 205 && newhsv.Y > 0.65f ? Color.White : Color.Black;
            Render2D.DrawLine(newColorRect.UpperLeft, newColorRect.UpperRight, newOutlineColor, 0.5f);
            Render2D.DrawLine(newColorRect.UpperRight, newColorRect.BottomRight, newOutlineColor, 0.5f);
            Render2D.DrawLine(newColorRect.BottomLeft, newColorRect.BottomRight, newOutlineColor, 0.5f);

            // Small separators between Old and New color preview
            Float2 separatorOffset = new Vector2(0, 9);

            Render2D.DrawLine(oldColorRect.UpperRight, oldColorRect.UpperRight + separatorOffset / 2, newOutlineColor, 0.5f);
            Render2D.DrawLine(oldColorRect.BottomRight, oldColorRect.BottomRight - separatorOffset / 2, newOutlineColor, 0.5f);

            // Tabs bounds debug
            //Render2D.DrawRectangle(_chsvRGBTabs.Bounds, Color.Green, 1);

            // Selector bounds debug
            //Render2D.DrawRectangle(_cSelector.Bounds, Color.Green, 1);
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            // Auto cancel on lost focus
#if !PLATFORM_LINUX
            ((WindowRootControl)Root).Window.LostFocus += OnSubmit;
#endif

            base.OnShow();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (key == KeyboardKeys.Escape)
            {
                if (_isEyedropperActive)
                {
                    // Cancel eye dropping
                    _isEyedropperActive = false;
                    _cEyedropper.BackgroundColor = Style.Current.Foreground;
                    _cEyedropper.BorderColor = Color.Transparent;
                    ScreenUtilities.PickColorDone -= OnColorPicked;

                    _onChanged?.Invoke(_eyedropperStartColor, false);


                    // TODO: Focus the dialog on eyedropper esc

                    return true;
                }

                // Restore color if modified
                if (_useDynamicEditing && _initialValue != _value)
                    _onChanged?.Invoke(_initialValue, false);
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            var child = GetChildAtRecursive(location);
            if (button == MouseButton.Right && child is Button b && b.Tag is Color c)
            {
                var menu = new ContextMenu.ContextMenu();
                var replaceButton = menu.AddButton("Replace");
                replaceButton.Clicked += () => OnSavedColorReplace(b);
                var deleteButton = menu.AddButton("Delete");
                deleteButton.Clicked += () => OnSavedColorDelete(b);
                _disableEvents = true;
                menu.Show(this, location);
                menu.VisibleChanged += (c) => _disableEvents = false;
                return true;
            }

            if (button == MouseButton.Left && oldColorRect.Contains(location))
            {
                _cSelector.Color = _initialValue;
                return true;
            }

            return false;
        }

        private void OnSavedColorReplace(Button button)
        {
            // Prevent setting same color 2 times... cause why...
            if (_savedColors.Any(c => c == _value))
                return;

            // Set new Color in spot
            for (int i = 0; i < _savedColors.Count; i++)
            {
                var color = _savedColors[i];
                if (color == (Color)button.Tag)
                    color = _value;
            }

            // Set color of button to current value;
            button.BackgroundColor = _value;
            button.BackgroundColorHighlighted = _value;
            button.Text = "";
            button.Tag = _value;

            // Save new colors
            var savedColors = JsonSerializer.Serialize(_savedColors, typeof(List<Color>));
            Editor.Instance.ProjectCache.SetCustomData("ColorPickerSavedColors", savedColors);
        }

        private void OnSavedColorDelete(Button button)
        {
            _savedColors.Remove((Color)button.Tag);

            foreach (var b in _savedColorButtons)
            {
                Children.Remove(b);
            }
            _savedColorButtons.Clear();

            CreateSavedSaveColorButtons();

            // Save new colors
            var savedColors = JsonSerializer.Serialize(_savedColors, typeof(List<Color>));
            Editor.Instance.ProjectCache.SetCustomData("ColorPickerSavedColors", savedColors);
        }

        private void CreateSavedSaveColorButtons()
        {
            float savedColorsYPosition = _cSelector.Bottom + LargeMargin;

            // Create buttons for saved colors
            for (int i = 0; i < _savedColors.Count; i++)
            {
                bool savedGreaterThanHalfMax = i + 1 > maxSavedColorsAmount / 2;

                float buttonYPosition = savedGreaterThanHalfMax ? savedColorsYPosition + SavedColorButtonHeight + SmallMargin : savedColorsYPosition;
                float buttonXPositionOffset = savedGreaterThanHalfMax ? (SavedColorButtonWidth + SmallMargin) * Mathf.Abs(maxSavedColorsAmount / 2 - i) : (SavedColorButtonWidth + SmallMargin) * i;

                var savedColor = _savedColors[i];       
                var savedColorButton = new SavedColorButton(_cSelector.Location.X + buttonXPositionOffset, buttonYPosition, SavedColorButtonWidth, SavedColorButtonHeight)
                {
                    Parent = this,
                    Tag = savedColor,
                    BackgroundColor = savedColor,
                    BackgroundColorHighlighted = savedColor,
                    TooltipText = $"{SavedColorButtonTooltip} {i + 1}.",
                    BackgroundColorSelected = savedColor.RGBMultiplied(0.8f),
                };
                savedColorButton.ButtonClicked += (b) => OnSavedColorButtonClicked(b);
                _savedColorButtons.Add(savedColorButton);
            }

            // Create "+" button to save color
            if (_savedColors.Count < maxSavedColorsAmount)
                CreateSaveNewColorButton(savedColorsYPosition);
        }

        private void CreateSaveNewColorButton(float savedColorsYPosition)
        {
            bool savedGreaterThanHalfMax = _savedColors.Count + 1 > maxSavedColorsAmount / 2;

            float buttonYPosition = savedGreaterThanHalfMax ? savedColorsYPosition + SavedColorButtonHeight + SmallMargin : savedColorsYPosition;
            float buttonXPositionOffset = savedGreaterThanHalfMax ? (SavedColorButtonWidth + SmallMargin) * Mathf.Abs(maxSavedColorsAmount / 2 - _savedColors.Count) : (SavedColorButtonWidth + SmallMargin) * _savedColors.Count;

            var savedColorButton = new SavedColorButton(_cSelector.Location.X + buttonXPositionOffset, buttonYPosition, SavedColorButtonWidth, SavedColorButtonHeight)
            {
                Text = "+",
                Parent = this,
                TooltipText = AddSavedColorButtonTooltip,
                Tag = null,
            };
            savedColorButton.ButtonClicked += (b) => OnSavedColorButtonClicked(b);
            _savedColorButtons.Add(savedColorButton);
        }

        private void UpdateAddSavedColorsButton()
        {
            // Make not being able to save the same color twice a bit more intuitive by disabeling the button when the color is already saved
            if (_savedColors.Contains(_value))
            {
                if (_savedColorButtons.Last().Tag == null)
                    _savedColorButtons.Last().Enabled = false;
                else
                    _savedColorButtons.Last().Enabled = true;
            }
            else
                _savedColorButtons.Last().Enabled = true;
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            if (_disableEvents)
                return;
            _disableEvents = true;

            // Send color event if modified
            if (_value != _initialValue)
                _onChanged?.Invoke(_value, false);

            base.OnSubmit();
        }

        /// <inheritdoc />
        public override void OnCancel()
        {
            if (_disableEvents)
                return;
            _disableEvents = true;

            // Restore color if modified
            if (_useDynamicEditing && _initialValue != _value)
                _onChanged?.Invoke(_initialValue, false);

            base.OnCancel();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _onClosed?.Invoke();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public void ClosePicker()
        {
            OnCancel();
        }
    }
}
