using System;
using System.Collections.Generic;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport.Widgets
{
    /// <summary>
    /// [todo] add summary
    /// </summary>
    public class ViewportWidgetButtonHorizontalGroup : HorizontalPanel
    {
        /// <summary>
        /// [todo] add summary
        /// </summary>
        /// <param name="buttons"></param>
        public ViewportWidgetButtonHorizontalGroup(ViewportWidgetButton[] buttons)
        {
            Margin = Margin.Zero;
            if (buttons.Length == 1)
            {
                buttons[0].BackgraundPlanel = Editor.Instance.Icons.Panel32;
                buttons[0].Parent = this;
            }
            else
            {
                buttons[0].BackgraundPlanel = Editor.Instance.Icons.PanelLeft32;
                buttons[0].Parent = this;
                for (int i = 1; i < buttons.Length - 1; i++)
                {
                    buttons[i].BackgraundPlanel = Editor.Instance.Icons.PanelHrizontal32;
                    buttons[i].Parent = this;
                }
                buttons[^1].BackgraundPlanel = Editor.Instance.Icons.PanelRight32;
                buttons[^1].Parent = this;
            }
            //PerformLayout();
        }
    }
}
