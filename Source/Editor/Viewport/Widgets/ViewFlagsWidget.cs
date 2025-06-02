using FlaxEditor.Content.Settings;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;
using JsonSerializer = FlaxEngine.Json.JsonSerializer;

namespace FlaxEditor.Viewport.Widgets
{

    public class ViewFlagsWidget : ViewportWidget
    {
        public ViewFlagsWidget(float x, float y, EditorViewport viewport, ContextMenu contextMenu)
        : base(x, y, 64, 32, viewport, contextMenu)
        {
        }

        protected override void OnAttach()
        {
            var viewFlags = ContextMenu.AddChildMenu("View Flags").ContextMenu;
            viewFlags.AddButton("Copy flags", () => Clipboard.Text = JsonSerializer.Serialize(Viewport.Task.ViewFlags));
            viewFlags.AddButton("Paste flags", () =>
            {
                try
                {
                    Viewport.Task.ViewFlags = JsonSerializer.Deserialize<ViewFlags>(Clipboard.Text);
                }
                catch
                {
                }
            });
            viewFlags.AddButton("Reset flags", () => Viewport.Task.ViewFlags = ViewFlags.DefaultEditor).Icon = Editor.Instance.Icons.Rotate32;
            viewFlags.AddButton("Disable flags", () => Viewport.Task.ViewFlags = ViewFlags.None).Icon = Editor.Instance.Icons.Rotate32;
            viewFlags.AddSeparator();
            for (int i = 0; i < ViewFlagsValues.Length; i++)
            {
                var v = ViewFlagsValues[i];
                var button = viewFlags.AddButton(v.Name);
                button.CloseMenuOnClick = false;
                button.Tag = v.Mode;
            }
            viewFlags.ButtonClicked += button =>
            {
                if (button.Tag != null)
                {
                    var v = (ViewFlags)button.Tag;
                    Viewport.Task.ViewFlags ^= v;
                    button.Icon = (Viewport.Task.ViewFlags & v) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                }
            };
            viewFlags.VisibleChanged += WidgetViewFlagsShowHide;
        }
        private static readonly ViewFlagOptions[] ViewFlagsValues =
{
            new ViewFlagOptions(ViewFlags.AntiAliasing, "Anti Aliasing"),
            new ViewFlagOptions(ViewFlags.Shadows, "Shadows"),
            new ViewFlagOptions(ViewFlags.EditorSprites, "Editor Sprites"),
            new ViewFlagOptions(ViewFlags.Reflections, "Reflections"),
            new ViewFlagOptions(ViewFlags.SSR, "Screen Space Reflections"),
            new ViewFlagOptions(ViewFlags.AO, "Ambient Occlusion"),
            new ViewFlagOptions(ViewFlags.GI, "Global Illumination"),
            new ViewFlagOptions(ViewFlags.DirectionalLights, "Directional Lights"),
            new ViewFlagOptions(ViewFlags.PointLights, "Point Lights"),
            new ViewFlagOptions(ViewFlags.SpotLights, "Spot Lights"),
            new ViewFlagOptions(ViewFlags.SkyLights, "Sky Lights"),
            new ViewFlagOptions(ViewFlags.Sky, "Sky"),
            new ViewFlagOptions(ViewFlags.Fog, "Fog"),
            new ViewFlagOptions(ViewFlags.SpecularLight, "Specular Light"),
            new ViewFlagOptions(ViewFlags.Decals, "Decals"),
            new ViewFlagOptions(ViewFlags.CustomPostProcess, "Custom Post Process"),
            new ViewFlagOptions(ViewFlags.Bloom, "Bloom"),
            new ViewFlagOptions(ViewFlags.ToneMapping, "Tone Mapping"),
            new ViewFlagOptions(ViewFlags.EyeAdaptation, "Eye Adaptation"),
            new ViewFlagOptions(ViewFlags.CameraArtifacts, "Camera Artifacts"),
            new ViewFlagOptions(ViewFlags.LensFlares, "Lens Flares"),
            new ViewFlagOptions(ViewFlags.DepthOfField, "Depth of Field"),
            new ViewFlagOptions(ViewFlags.MotionBlur, "Motion Blur"),
            new ViewFlagOptions(ViewFlags.ContactShadows, "Contact Shadows"),
            new ViewFlagOptions(ViewFlags.PhysicsDebug, "Physics Debug"),
            new ViewFlagOptions(ViewFlags.LightsDebug, "Lights Debug"),
            new ViewFlagOptions(ViewFlags.DebugDraw, "Debug Draw"),
        };

        private struct ViewFlagOptions
        {
            public readonly ViewFlags Mode;
            public readonly string Name;

            public ViewFlagOptions(ViewFlags mode, string name)
            {
                Mode = mode;
                Name = name;
            }
        }

        private void WidgetViewFlagsShowHide(Control cm)
        {
            if (cm.Visible == false)
                return;
            var ccm = (ContextMenu)cm;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b && b.Tag != null)
                {
                    var v = (ViewFlags)b.Tag;
                    b.Icon = (Viewport.Task.View.Flags & v) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                }
            }
        }
    }
}