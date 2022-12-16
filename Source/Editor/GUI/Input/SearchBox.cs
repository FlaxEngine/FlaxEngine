using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Input
{
    /// <summary>
    /// Search box control which can gather text search input from the user.
    /// </summary>
    public class SearchBox : TextBox
    {
        /// <summary>
        /// A button that clears the search bar.
        /// </summary>
        public Button ClearSearchButton { get; }

        private Color _defaultButtonTextColor;
        
        /// <summary>
        /// Init search box
        /// </summary>
        public SearchBox()
        : this(false, 0, 0)
        {
        }
        
        /// <summary>
        /// Init search box
        /// </summary>
        public SearchBox(bool isMultiline, float x, float y, float width = 120)
        : base(isMultiline, x, y, width)
        {
            WatermarkText = "Search...";
            
            ClearSearchButton = new Button
            {
                Parent = this,
                Width = 15.0f,
                Height = 14.0f,
                AnchorPreset = AnchorPresets.TopRight,
                Text = "X",
                TooltipText = "Cancel Search.",
                BackgroundColor = Color.Transparent,
                BorderColor = Color.Transparent,
                BackgroundColorHighlighted = Color.Transparent,
                BorderColorHighlighted = Color.Transparent,
                BackgroundColorSelected = Color.Transparent,
                BorderColorSelected = Color.Transparent,
                Visible = false,
            };
            ClearSearchButton.LocalY += 2;
            ClearSearchButton.LocalX -= 2;
            ClearSearchButton.Clicked += Clear;
            _defaultButtonTextColor = ClearSearchButton.TextColor;

            TextChanged += () => ClearSearchButton.Visible = !string.IsNullOrEmpty(Text);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            ClearSearchButton.TextColor = ClearSearchButton.IsMouseOver ? Style.Current.ForegroundGrey : _defaultButtonTextColor;
        }
    }
}
