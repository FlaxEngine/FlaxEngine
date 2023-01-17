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
        private Label _timeLabel;
        private Button _saveNowButton;
        private Button _cancelSaveButton;
        private Color _defaultTextColor;
        private bool _isMoved = false;
        
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
                BackgroundColor = Color.Transparent,
            };
            
            _timeLabel = new Label(0, 0, 25, 10)
            {
                Parent = _backgroundPanel,
                Text = _timeRemaining.ToString(),
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                AutoWidth = true,
                Height = 14,
                AnchorPreset = AnchorPresets.MiddleLeft,
            };

            _saveNowButton = new Button
            {
                Parent = _backgroundPanel,
                Height = 14,
                Width = 60,
                AnchorPreset = AnchorPresets.MiddleRight,
                BackgroundColor = Color.Transparent,
                BorderColor = Color.Transparent,
                BackgroundColorHighlighted = Color.Transparent,
                BackgroundColorSelected = Color.Transparent,
                BorderColorHighlighted = Color.Transparent,
                Text = "Save Now",
                TooltipText = "Saves now and restarts the auto save timer."
            };
            _saveNowButton.LocalX -= 85;
            
            _cancelSaveButton = new Button
            {
                Parent = _backgroundPanel,
                Height = 14,
                Width = 70,
                AnchorPreset = AnchorPresets.MiddleRight,
                BackgroundColor = Color.Transparent,
                BorderColor = Color.Transparent,
                BackgroundColorHighlighted = Color.Transparent,
                BackgroundColorSelected = Color.Transparent,
                BorderColorHighlighted = Color.Transparent,
                Text = "Cancel Save",
                TooltipText = "Cancels this auto save."
            };
            _cancelSaveButton.LocalX -= 5;

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
                _timeLabel.Text = "Auto Save in: " + _timeRemaining;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (IsMouseOver)
            {
                _saveNowButton.TextColor = _saveNowButton.IsMouseOver ? Style.Current.BackgroundHighlighted : _defaultTextColor;
                _cancelSaveButton.TextColor = _cancelSaveButton.IsMouseOver ? Style.Current.BackgroundHighlighted : _defaultTextColor;
            }
            
            // Move if the progress bar is visible
            if (Editor.Instance.UI.ProgressVisible && !_isMoved)
            {
                _isMoved = true;
                LocalX -= 280;
            }
            else if (!Editor.Instance.UI.ProgressVisible && _isMoved)
            {
                LocalX += 280;
                _isMoved = false;
            }
            base.Update(deltaTime);
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
                Height = Editor.Instance.UI.StatusBar.Height,
                Width = 250,
                AnchorPreset = AnchorPresets.BottomRight,
            };
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
