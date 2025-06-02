using System.Collections.Generic;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport.Widgets
{

    public class FpsCounterWidget : ViewportWidget
    {
        public FpsCounterWidget(float x, float y, EditorViewport viewport, ContextMenu control)
        : base(x, y, 64, 32, viewport, control)
        {
        }

        public override void Draw()
        {
            base.Draw();

            int fps = Engine.FramesPerSecond;
            Color color = Color.Green;
            if (fps < 13)
                color = Color.Red;
            else if (fps < 22)
                color = Color.Yellow;
            var text = string.Format("FPS: {0}", fps);
            var font = Style.Current.FontMedium;
            Render2D.DrawText(font, text, new Rectangle(Float2.One, Size), Color.Black);
            Render2D.DrawText(font, text, new Rectangle(Float2.Zero, Size), color);
        }

        private ContextMenuButton _showFpsButton;

        /// <summary>
        /// Gets or sets a value indicating whether show or hide FPS counter.
        /// </summary>
        public bool ShowFpsCounter
        {
            get => Visible;
            set
            {
                Visible = value;
                Enabled = value;
                _showFpsButton.Icon = value ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
            }
        }

        protected override void OnAttach(){
                Visible = false;
                Enabled = false;
                var fpsMenu = ContextMenu.AddChildMenu("Show").ContextMenu;
                _showFpsButton = fpsMenu.AddButton("FPS Counter", () => ShowFpsCounter = !ShowFpsCounter);
                _showFpsButton.CloseMenuOnClick = false;
            }
        }
    }
