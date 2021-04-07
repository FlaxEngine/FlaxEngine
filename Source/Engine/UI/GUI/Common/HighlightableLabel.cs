using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FlaxEngine.GUI
{
    /// <inheritdoc/>
    [HideInEditor]
    public class HighlightableLabel : Label
    {
        /// <inheritdoc/>
        public HighlightableLabel()
        : base(0, 0, 100, 20)
        {
            TextColorHighlighted = Style.Current.Foreground;
        }

        /// <inheritdoc/>
        public HighlightableLabel(float x, float y, float width, float height)
        : base(x, y, width, height)
        {
            TextColorHighlighted = Style.Current.Foreground;
        }

        /// <inheritdoc/>
        public override Color GetColorToDraw()
        {
            return IsMouseOver ? TextColorHighlighted : base.GetColorToDraw();
        }
        /// <summary>
        /// Gets or sets the color of the text when it is highlighted (mouse is over).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color of the text when it is highlighted (mouse is over).")]
        public Color TextColorHighlighted { get; set; }
    }
}
