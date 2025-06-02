using System.Collections.Generic;
using System.Reflection;
using System.Xml;
using FlaxEditor.Content.Settings;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;
namespace FlaxEditor.Viewport.Widgets
{

    public class ViewmodeWidget : ViewportWidget
    {
        /// <summary>
        /// The 'View' widget button context menu.
        /// </summary>
        public ContextMenu widgetMenu;

        /// <summary>
        /// The 'View' widget 'Show' category context menu.
        /// </summary>
        public ContextMenu ViewWidgetShowMenu;

        public ViewmodeWidget(float x, float y, EditorViewport viewport)
        : base(x, y, 64, 32, viewport)
        {
        }

        protected override void OnAttach()
        {
            var widgetContainer = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft)
            {
                Parent = Viewport
            };
            widgetMenu = new ContextMenu();
            //this needs to hide better
            var viewModeButton = new ViewportWidgetButton("View", SpriteHandle.Invalid, widgetMenu)
            {
                TooltipText = "View properties",
                Parent = widgetContainer,
            };

            //Show FPS
            new FpsCounterWidget(0, 0, Viewport, widgetMenu);

            //View Layers
            new ViewLayersWidget(0, 0, Viewport, widgetMenu);

            //View Flags
            new ViewFlagsWidget(0, 0, Viewport, widgetMenu);

            //Debug View
            new DebugViewWidget(0, 0, Viewport, widgetMenu);

            widgetMenu.AddSeparator();

            var sliderOffset = Style.Current.FontMedium.MeasureText("Brightness").X + 5;
            //Brightness
            new BrightnessWidget(sliderOffset, Viewport, widgetMenu);

            //Resolution
            new ResolutionWidget(sliderOffset, Viewport, widgetMenu);

            widgetMenu.AddSeparator();

            //todo no idea how to test this dont know when it is shown
            // Clear Debug Draw
            {
                var button = widgetMenu.AddButton("Clear Debug Draw");
                button.CloseMenuOnClick = false;
                button.Clicked += () => DebugDraw.Clear();
                widgetMenu.VisibleChanged += (Control cm) => { button.Visible = DebugDraw.CanClear(); };
            }
        }
    }
}