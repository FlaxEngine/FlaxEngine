using FlaxEditor.GUI;
using FlaxEngine;

namespace FlaxEditor.Content.GUI
{
    internal class ContentNavigationBar : NavigationBar
    {
        ToolStrip _toolstrip;
        internal float ofssetFromRightEdge = 80;
        internal ContentNavigationBar(ToolStrip toolstrip) : base()
        {
            _toolstrip = toolstrip;
        }
        /// <inheritdoc />
        protected override void Arrange()
        {
            base.Arrange();
            var lastToolstripButton = _toolstrip.LastButton;
            var parentSize = Parent.Size;
            Bounds = new Rectangle
            (
             new Float2(lastToolstripButton.Right, 0),
             new Float2(parentSize.X - X - ofssetFromRightEdge, _toolstrip.Height)
            );
        }
    }
}
