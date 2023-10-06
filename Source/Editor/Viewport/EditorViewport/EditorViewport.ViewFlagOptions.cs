using FlaxEngine;

namespace FlaxEditor.Viewport
{
    /// <summary>
    /// Editor viewports base class.
    /// </summary>
    /// <seealso cref="T:FlaxEngine.GUI.RenderOutputControl" />
    public partial class EditorViewport
    {
        private static readonly ViewFlagOptions[] EditorViewportViewFlagsValues = new ViewFlagOptions[]
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
            new ViewFlagOptions(ViewFlags.DebugDraw, "Debug Draw")
};

        private struct ViewFlagOptions
        {
            public ViewFlagOptions(ViewFlags mode, string name)
            {
                Mode = mode;
                Name = name;
            }

            public readonly ViewFlags Mode;

            public readonly string Name;
        }
    }
}
