using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Popup menu for reminding of an upcoming auto save.
    /// </summary>
    public class AutoSavePopup : ContainerControl
    {
        /// <summary>
        /// Gets or sets the value for if the user has manually closed this popup.
        /// </summary>
        public bool UserClosed { get; set; }
        
        /// <summary>
        /// The save now button. Used to skip the last remaining auto save time.
        /// </summary>
        public Button SaveNowButton => _saveNowButton;
        
        /// <summary>
        /// Button that should be used to cancel the auto save.
        /// </summary>
        public Button CancelSaveButton => _cancelSaveButton;

        private int _timeRemaining;
        private Panel _backgroundPanel;
        private Label _autoSaveLabel;
        private Label _timeRemainingLabel;
        private Label _timeLabel;
        private Button _saveNowButton;
        private Button _cancelSaveButton;
        private Button _closeMenuButton;
        private Color _defaultTextColor;
        
        /// <summary>
        /// Initialize the AutoSavePopup.
        /// </summary>
        /// <param name="initialTime">The initial time to set the time label to.</param>
        public AutoSavePopup(float initialTime)
        {
            UpdateTime(initialTime);

            _backgroundPanel = new Panel(ScrollBars.None)
            {
                Parent = this,
                AnchorPreset = AnchorPresets.StretchAll,
            };
            
            _autoSaveLabel = new Label(0, 0, 25, 10)
            {
                Parent = _backgroundPanel,
                Text = "Auto Save",
                AutoWidth = true,
                AutoHeight = true,
                AnchorPreset = AnchorPresets.TopLeft,
            };
            _autoSaveLabel.Font.Size = 16;
            _autoSaveLabel.LocalX += 5;
            _autoSaveLabel.LocalY += 5;

            _timeRemainingLabel = new Label(0, 0, 25, 10)
            {
                Parent = _backgroundPanel,
                Text = "Time Remaining: ",
                AutoWidth = true,
                AutoHeight = true,
                AnchorPreset = AnchorPresets.MiddleLeft,
            };
            _timeRemainingLabel.Font.Size = 16;
            _timeRemainingLabel.LocalX += 5;
            _timeRemainingLabel.LocalY -= _timeRemainingLabel.Height / 2;
            
            _timeLabel = new Label(0, 0, 25, 10)
            {
                Parent = _backgroundPanel,
                Text = _timeRemaining.ToString(),
                HorizontalAlignment = TextAlignment.Far,
                VerticalAlignment = TextAlignment.Center,
                Width = 25,
                AutoHeight = true,
                AnchorPreset = AnchorPresets.MiddleRight,
            };
            _timeLabel.Font.Size = 16;
            _timeLabel.LocalX -= 5;
            _timeLabel.LocalY -= _timeLabel.Height / 2;

            _saveNowButton = new Button
            {
                Parent = _backgroundPanel,
                Height = 14,
                Width = 60,
                AnchorPreset = AnchorPresets.BottomLeft,
                BackgroundColor = Color.Transparent,
                BorderColor = Color.Transparent,
                BackgroundColorHighlighted = Color.Transparent,
                BackgroundColorSelected = Color.Transparent,
                BorderColorHighlighted = Color.Transparent,
                Text = "Save Now",
                TooltipText = "Saves now and restarts auto save timer."
            };
            _saveNowButton.LocalY -= 5;
            _saveNowButton.LocalX += 5;
            
            _cancelSaveButton = new Button
            {
                Parent = _backgroundPanel,
                Height = 14,
                Width = 70,
                AnchorPreset = AnchorPresets.BottomRight,
                BackgroundColor = Color.Transparent,
                BorderColor = Color.Transparent,
                BackgroundColorHighlighted = Color.Transparent,
                BackgroundColorSelected = Color.Transparent,
                BorderColorHighlighted = Color.Transparent,
                Text = "Cancel Save",
                TooltipText = "Cancels this auto save."
            };
            _cancelSaveButton.LocalY -= 5;
            _cancelSaveButton.LocalX -= 5;

            _closeMenuButton = new Button
            {
                Parent = _backgroundPanel,
                Height = 14,
                Width = 14,
                AnchorPreset = AnchorPresets.TopRight,
                BackgroundBrush = new SpriteBrush(Style.Current.Cross),
                BorderColor = Color.Transparent,
                BackgroundColorHighlighted = Style.Current.ForegroundGrey,
                BackgroundColorSelected = Color.Transparent,
                BorderColorHighlighted = Color.Transparent,
                BorderColorSelected = Color.Transparent,
                TooltipText = "Close Popup."
            };
            _closeMenuButton.LocalY += 5;
            _closeMenuButton.LocalX -= 5;
            _closeMenuButton.BackgroundColor = _closeMenuButton.TextColor;
            _closeMenuButton.Clicked += () =>
            {
                UserClosed = true;
                HidePopup();
            };

            _defaultTextColor = _saveNowButton.TextColor;
        }

        /// <summary>
        /// Updates the time label
        /// </summary>
        /// <param name="time">The time to change the label to.</param>
        public void UpdateTime(float time)
        {
            _timeRemaining = Mathf.CeilToInt(time);
            if (_timeLabel != null)
                _timeLabel.Text = _timeRemaining.ToString();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (IsMouseOver)
            {
                _saveNowButton.TextColor = _saveNowButton.IsMouseOver ? Style.Current.BackgroundHighlighted : _defaultTextColor;
                _cancelSaveButton.TextColor = _cancelSaveButton.IsMouseOver ? Style.Current.BackgroundHighlighted : _defaultTextColor;
            }
            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Draw background
            var style = Style.Current;
            var bounds = new Rectangle(Float2.Zero, Size);
            Render2D.FillRectangle(bounds, style.Background);
            Render2D.DrawRectangle(bounds, Color.Lerp(style.BackgroundSelected, style.Background, 0.6f));

            base.Draw();
        }

        /// <summary>
        /// Creates and shows the AutoSavePopup
        /// </summary>
        /// <param name="parentControl">The parent control.</param>
        /// <param name="initialTime">The time to start at.</param>
        /// <returns></returns>
        public static AutoSavePopup Show(ContainerControl parentControl, float initialTime)
        {
            var popup = new AutoSavePopup(initialTime)
            {
                Parent = parentControl,
                Height = 75,
                Width = 250,
                AnchorPreset = AnchorPresets.BottomRight,
            };
            popup.LocalX -= 10;
            popup.LocalY -= 30;
            popup.ShowPopup();
            return popup;
        }

        /// <summary>
        /// Shows the popup by changing its visibility
        /// </summary>
        public void ShowPopup()
        {
            Visible = true;
            UserClosed = false;
            _saveNowButton.TextColor = _defaultTextColor;
            _cancelSaveButton.TextColor = _defaultTextColor;
        }

        /// <summary>
        /// Hides the popup by changing its visibility
        /// </summary>
        public void HidePopup()
        {
            Visible = false;
            if (_containsFocus)
                Defocus();
        }
    }
}
