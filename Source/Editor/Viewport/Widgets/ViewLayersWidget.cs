using FlaxEditor.Content.Settings;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;
using JsonSerializer = FlaxEngine.Json.JsonSerializer;

namespace FlaxEditor.Viewport.Widgets
{

    public class ViewLayersWidget : ViewportWidget
    {

        public ViewLayersWidget(float x, float y, EditorViewport viewport, ContextMenu contextMenu)
        : base(x, y, 64, 32, viewport, contextMenu)
        {
        }

        protected override void OnAttach()
        {
            // View Layers
            var viewLayers = ContextMenu.AddChildMenu("View Layers").ContextMenu;
            viewLayers.AddButton("Copy layers", () => Clipboard.Text = JsonSerializer.Serialize(Viewport.Task.View.RenderLayersMask));
            viewLayers.AddButton("Paste layers", () =>
            {
                try
                {
                    Viewport.Task.ViewLayersMask = JsonSerializer.Deserialize<LayersMask>(Clipboard.Text);
                }
                catch
                {
                }
            });
            viewLayers.AddButton("Reset layers", () => Viewport.Task.ViewLayersMask = LayersMask.Default).Icon = Editor.Instance.Icons.Rotate32;
            viewLayers.AddButton("Disable layers", () => Viewport.Task.ViewLayersMask = new LayersMask(0)).Icon = Editor.Instance.Icons.Rotate32;
            viewLayers.AddSeparator();
            var layers = LayersAndTagsSettings.GetCurrentLayers();
            if (layers != null && layers.Length > 0)
            {
                for (int i = 0; i < layers.Length; i++)
                {
                    var layer = layers[i];
                    var button = viewLayers.AddButton(layer);
                    button.CloseMenuOnClick = false;
                    button.Tag = 1 << i;
                }
            }
            viewLayers.ButtonClicked += button =>
            {
                if (button.Tag != null)
                {
                    int layerIndex = (int)button.Tag;
                    LayersMask mask = new LayersMask(layerIndex);
                    Viewport.Task.ViewLayersMask ^= mask;
                    button.Icon = (Viewport.Task.ViewLayersMask & mask) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                }
            };
            viewLayers.VisibleChanged += WidgetViewLayersShowHide;
        }

        private void WidgetViewLayersShowHide(Control cm)
        {
            if (cm.Visible == false)
                return;
            var ccm = (ContextMenu)cm;
            var layersMask = Viewport.Task.ViewLayersMask;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b && b != null && b.Tag != null)
                {
                    int layerIndex = (int)b.Tag;
                    LayersMask mask = new LayersMask(layerIndex);
                    b.Icon = (layersMask & mask) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                }
            }
        }
    }
}