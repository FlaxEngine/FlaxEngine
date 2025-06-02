using System.Collections.Generic;
using FlaxEditor.Content.Settings;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;
using JsonSerializer = FlaxEngine.Json.JsonSerializer;

namespace FlaxEditor.Viewport.Widgets
{

    public class DebugViewWidget : ViewportWidget
    {
        private ContextMenu _debugView;
        public DebugViewWidget(float x, float y, EditorViewport viewport, ContextMenu contextMenu)
        : base(x, y, 64, 32, viewport, contextMenu)
        {
        }

        protected override void OnAttach()
        {
            _debugView = ContextMenu.AddChildMenu("Debug View").ContextMenu;
            _debugView.AddButton("Copy view", () => Clipboard.Text = JsonSerializer.Serialize(Viewport.Task.ViewMode));
            _debugView.AddButton("Paste view", () =>
            {
                try
                {
                    Viewport.Task.ViewMode = JsonSerializer.Deserialize<ViewMode>(Clipboard.Text);
                }
                catch
                {
                }
            });
            _debugView.AddSeparator();
            for (int i = 0; i < ViewModeValues.Length; i++)
            {
                ref var v = ref ViewModeValues[i];
                if (v.Options != null)
                {
                    var childMenu = _debugView.AddChildMenu(v.Name).ContextMenu;
                    childMenu.ButtonClicked += WidgetViewModeShowHideClicked;
                    childMenu.VisibleChanged += WidgetViewModeShowHide;
                    for (int j = 0; j < v.Options.Length; j++)
                    {
                        ref var vv = ref v.Options[j];
                        var button = childMenu.AddButton(vv.Name);
                        button.CloseMenuOnClick = false;
                        button.Tag = vv.Mode;
                    }
                }
                else
                {
                    var button = _debugView.AddButton(v.Name);
                    button.CloseMenuOnClick = false;
                    button.Tag = v.Mode;
                }
            }
            _debugView.ButtonClicked += WidgetViewModeShowHideClicked;
            _debugView.VisibleChanged += WidgetViewModeShowHide;
        }

        private void WidgetViewModeShowHideClicked(ContextMenuButton button)
        {
            if (button.Tag is ViewMode v)
            {
                Viewport.Task.ViewMode = v;
                var cm = button.ParentContextMenu;
                WidgetViewModeShowHide(cm);
                if (_debugView != null && cm != _debugView)
                    WidgetViewModeShowHide(_debugView);
            }
        }

        private void WidgetViewModeShowHide(Control cm)
        {
            if (cm.Visible == false)
                return;

            var ccm = (ContextMenu)cm;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b && b.Tag is ViewMode v)
                    b.Icon = Viewport.Task.ViewMode == v ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
            }
        }
        private static readonly ViewModeOptions[] ViewModeValues =
        {
            new ViewModeOptions(ViewMode.Default, "Default"),
            new ViewModeOptions(ViewMode.Unlit, "Unlit"),
            new ViewModeOptions(ViewMode.NoPostFx, "No PostFx"),
            new ViewModeOptions(ViewMode.Wireframe, "Wireframe"),
            new ViewModeOptions(ViewMode.LightBuffer, "Light Buffer"),
            new ViewModeOptions(ViewMode.Reflections, "Reflections Buffer"),
            new ViewModeOptions(ViewMode.Depth, "Depth Buffer"),
            new ViewModeOptions("GBuffer", new[]
            {
                new ViewModeOptions(ViewMode.Diffuse, "Diffuse"),
                new ViewModeOptions(ViewMode.Metalness, "Metalness"),
                new ViewModeOptions(ViewMode.Roughness, "Roughness"),
                new ViewModeOptions(ViewMode.Specular, "Specular"),
                new ViewModeOptions(ViewMode.SpecularColor, "Specular Color"),
                new ViewModeOptions(ViewMode.SubsurfaceColor, "Subsurface Color"),
                new ViewModeOptions(ViewMode.ShadingModel, "Shading Model"),
                new ViewModeOptions(ViewMode.Emissive, "Emissive Light"),
                new ViewModeOptions(ViewMode.Normals, "Normals"),
                new ViewModeOptions(ViewMode.AmbientOcclusion, "Ambient Occlusion"),
            }),
            new ViewModeOptions(ViewMode.MotionVectors, "Motion Vectors"),
            new ViewModeOptions(ViewMode.LightmapUVsDensity, "Lightmap UVs Density"),
            new ViewModeOptions(ViewMode.VertexColors, "Vertex Colors"),
            new ViewModeOptions(ViewMode.PhysicsColliders, "Physics Colliders"),
            new ViewModeOptions(ViewMode.LODPreview, "LOD Preview"),
            new ViewModeOptions(ViewMode.MaterialComplexity, "Material Complexity"),
            new ViewModeOptions(ViewMode.QuadOverdraw, "Quad Overdraw"),
            new ViewModeOptions(ViewMode.GlobalSDF, "Global SDF"),
            new ViewModeOptions(ViewMode.GlobalSurfaceAtlas, "Global Surface Atlas"),
            new ViewModeOptions(ViewMode.GlobalIllumination, "Global Illumination"),
        };
        private struct ViewModeOptions
        {
            public readonly string Name;
            public readonly ViewMode Mode;
            public readonly ViewModeOptions[] Options;

            public ViewModeOptions(ViewMode mode, string name)
            {
                Mode = mode;
                Name = name;
                Options = null;
            }

            public ViewModeOptions(string name, ViewModeOptions[] options)
            {
                Name = name;
                Mode = ViewMode.Default;
                Options = options;
            }
        }
    }
}