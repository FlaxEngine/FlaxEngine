using FlaxEngine;

namespace FlaxEditor.Viewport
{
    /// <summary>
    /// Editor viewports base class.
    /// </summary>
    /// <seealso cref="T:FlaxEngine.GUI.RenderOutputControl" />
    public partial class EditorViewport
    {

        private static readonly ViewModeOptions[] EditorViewportViewModeValues = new ViewModeOptions[]
        {
            new ViewModeOptions(ViewMode.Default, "Default"),
            new ViewModeOptions(ViewMode.Unlit, "Unlit"),
            new ViewModeOptions(ViewMode.NoPostFx, "No PostFx"),
            new ViewModeOptions(ViewMode.Wireframe, "Wireframe"),
            new ViewModeOptions(ViewMode.LightBuffer, "Light Buffer"),
            new ViewModeOptions(ViewMode.Reflections, "Reflections Buffer"),
            new ViewModeOptions(ViewMode.Depth, "Depth Buffer"),
            new ViewModeOptions("GBuffer", new ViewModeOptions[]
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
                new ViewModeOptions(ViewMode.AmbientOcclusion, "Ambient Occlusion")
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
            new ViewModeOptions(ViewMode.GlobalIllumination, "Global Illumination")
        };

        private struct ViewModeOptions
        {
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

            public readonly string Name;

            public readonly ViewMode Mode;

            public readonly ViewModeOptions[] Options;
        }
    }
}
