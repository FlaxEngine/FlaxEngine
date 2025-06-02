using System.Collections.Generic;
using FlaxEditor.Content.Settings;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;
using JsonSerializer = FlaxEngine.Json.JsonSerializer;

namespace FlaxEditor.Viewport.Widgets
{

    public class BrightnessWidget : ViewportWidget
    {
        private float _sliderOffset;
        //todo hack for xLocationForWidgets
        public BrightnessWidget(float sliderOffset, EditorViewport viewport, ContextMenu contextMenu)
        : base(0, 0, 64, 32, viewport, contextMenu, true, false)
        {
            _sliderOffset = sliderOffset;
            OnAttach();
        }
        protected override void OnAttach()
        {
            var brightness = ContextMenu.AddButton("Brightness");
            brightness.CloseMenuOnClick = false;
            var brightnessValue = new FloatValueBox(1.0f, _sliderOffset, 2, 70.0f, 0.001f, 10.0f, 0.001f)
            {
                Parent = brightness
            };
            brightnessValue.ValueChanged += () => Viewport.Brightness = brightnessValue.Value;
            ContextMenu.VisibleChanged += control => brightnessValue.Value = Viewport.Brightness;
        }
    }
}