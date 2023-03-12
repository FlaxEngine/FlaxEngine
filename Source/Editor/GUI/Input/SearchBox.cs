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
                Width = 14.0f,
                Height = 14.0f,
                AnchorPreset = AnchorPresets.TopRight,
                Text = "",
                TooltipText = "Cancel Search.",
                BackgroundColor = TextColor,
                BorderColor = Color.Transparent,
                BackgroundColorHighlighted = Style.Current.ForegroundGrey,
                BorderColorHighlighted = Color.Transparent,
                BackgroundColorSelected = Style.Current.ForegroundGrey,
                BorderColorSelected = Color.Transparent,
                BackgroundBrush = new SpriteBrush(Editor.Instance.Icons.Cross12),
                Visible = false,
            };
            ClearSearchButton.LocalY += 2;
            ClearSearchButton.LocalX -= 2;
            ClearSearchButton.Clicked += Clear;
            ClearSearchButton.HoverBegin += () =>
            {
                _changeCursor = false;
                Cursor = CursorType.Default;
            };
            ClearSearchButton.HoverEnd += () => _changeCursor = true;

            TextChanged += () => ClearSearchButton.Visible = !string.IsNullOrEmpty(Text);
        }
    }
}
