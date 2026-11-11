using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Input
{
    /// <summary>
    /// Search box control which can gather text search input from the user.
    /// </summary>
    public class SearchBox : TextBox
    {
        private const float IconSize = 14.0f;

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
                Size = new Float2(IconSize),
                AnchorPreset = AnchorPresets.TopRight,
                Text = "",
                TooltipText = "Cancel search.",
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

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw search icon
            var style = Style.Current;
            var rect = new Rectangle(2, 2, IconSize, IconSize);
            Render2D.DrawSprite(style.Search, rect, EnabledInHierarchy ? style.Foreground : style.ForegroundDisabled);
        }

        /// <inheritdoc />
        protected override Rectangle TextRectangle
        {
            get
            {
                var result = base.TextRectangle;
                var searchIconOffset = IconSize + 2;
                result.Location.X += searchIconOffset;
                result.Size.X -= searchIconOffset;
                if (ClearSearchButton.Visible)
                    result.Size.X -= IconSize;
                return result;
            }
        }

        /// <inheritdoc />
        protected override Rectangle TextClipRectangle
        {
            get
            {
                var result = base.TextRectangle;
                if (ClearSearchButton.Visible)
                    result.Size.X -= IconSize;
                return result;
            }
        }
    }
}
