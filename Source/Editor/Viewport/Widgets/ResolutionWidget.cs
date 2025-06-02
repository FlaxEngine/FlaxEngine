using System.Collections.Generic;
using FlaxEditor.Content.Settings;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;
using JsonSerializer = FlaxEngine.Json.JsonSerializer;

namespace FlaxEditor.Viewport.Widgets
{

    public class ResolutionWidget : ViewportWidget
    {
        private float _sliderOffset;
        //todo hack for xLocationForWidgets
        public ResolutionWidget(float sliderOffset, EditorViewport viewport, ContextMenu contextMenu)
        : base(0, 0, 64, 32, viewport, contextMenu, true, false)
        {
            _sliderOffset = sliderOffset;
            OnAttach();
        }
        protected override void OnAttach()
        {
            var resolution = ContextMenu.AddButton("Resolution");
            resolution.CloseMenuOnClick = false;
            var resolutionValue = new FloatValueBox(1.0f, _sliderOffset, 2, 70.0f, 0.1f, 4.0f, 0.001f)
            {
                Parent = resolution
            };
            resolutionValue.ValueChanged += () => Viewport.ResolutionScale = resolutionValue.Value;
            ContextMenu.VisibleChanged += control => resolutionValue.Value = Viewport.ResolutionScale;
        }
    }
}