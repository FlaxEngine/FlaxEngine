// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using FlaxEngine;

namespace FlaxEditor.Content.Import
{
    /// <summary>
    /// Importing model lightmap UVs source
    /// </summary>
    [HideInEditor]
    public enum ModelLightmapUVsSource : int
    {
        /// <summary>
        /// No lightmap UVs.
        /// </summary>
        Disable = 0,

        /// <summary>
        /// Generate lightmap UVs from model geometry.
        /// </summary>
        Generate = 1,

        /// <summary>
        /// The texcoords channel 0.
        /// </summary>
        Channel0 = 2,

        /// <summary>
        /// The texcoords channel 1.
        /// </summary>
        Channel1 = 3,

        /// <summary>
        /// The texcoords channel 2.
        /// </summary>
        Channel2 = 4,

        /// <summary>
        /// The texcoords channel 3.
        /// </summary>
        Channel3 = 5
    }

    /// <summary>
    /// Declares the imported data type.
    /// </summary>
    [HideInEditor]
    public enum ModelType : int
    {
        /// <summary>
        /// The model asset.
        /// </summary>
        Model = 0,

        /// <summary>
        /// The skinned model asset.
        /// </summary>
        SkinnedModel = 1,

        /// <summary>
        /// The animation asset.
        /// </summary>
        Animation = 2,
    }

    /// <summary>
    /// Declares the imported animation clip duration.
    /// </summary>
    [HideInEditor]
    public enum AnimationDuration : int
    {
        /// <summary>
        /// The imported duration.
        /// </summary>
        Imported = 0,

        /// <summary>
        /// The custom duration specified via keyframes range.
        /// </summary>
        Custom = 1,
    }

    /// <summary>
    /// Proxy object to present model import settings in <see cref="ImportFilesDialog"/>.
    /// </summary>
    [HideInEditor]
    public class ModelImportSettings
    {
        /// <summary>
        /// Type of the imported asset.
        /// </summary>
        [EditorOrder(0)]
        public ModelType Type { get; set; } = ModelType.Model;

        /// <summary>
        /// Enable model normal vectors recalculating.
        /// </summary>
        [EditorDisplay("Geometry"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(20), DefaultValue(false)]
        public bool CalculateNormals { get; set; } = false;

        /// <summary>
        /// Specifies the maximum angle (in degrees) that may be between two face normals at the same vertex position that their are smoothed together. The default value is 175.
        /// </summary>
        [EditorDisplay("Geometry"), VisibleIf(nameof(ShowSmoothingNormalsAngle))]
        [EditorOrder(30), DefaultValue(175.0f), Limit(0, 175, 0.1f)]
        public float SmoothingNormalsAngle { get; set; } = 175.0f;

        private bool ShowSmoothingNormalsAngle => ShowGeometry && CalculateNormals;

        /// <summary>
        /// If checked, the imported normal vectors of the mesh will be flipped (scaled by -1).
        /// </summary>
        [EditorDisplay("Geometry"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(35), DefaultValue(false)]
        public bool FlipNormals { get; set; } = false;

        /// <summary>
        /// Enable model tangent vectors recalculating.
        /// </summary>
        [EditorDisplay("Geometry"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(40), DefaultValue(false)]
        public bool CalculateTangents { get; set; } = false;

        /// <summary>
        /// Specifies the maximum angle (in degrees) that may be between two vertex tangents that their tangents and bi-tangents are smoothed. The default value is 45.
        /// </summary>
        [EditorDisplay("Geometry"), VisibleIf(nameof(ShowSmoothingTangentsAngle))]
        [EditorOrder(45), DefaultValue(45.0f), Limit(0, 45, 0.1f)]
        public float SmoothingTangentsAngle { get; set; } = 45.0f;

        private bool ShowSmoothingTangentsAngle => ShowGeometry && CalculateTangents;

        /// <summary>
        /// Enable/disable meshes geometry optimization.
        /// </summary>
        [EditorDisplay("Geometry"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(50), DefaultValue(true)]
        public bool OptimizeMeshes { get; set; } = true;

        /// <summary>
        /// Enable/disable geometry merge for meshes with the same materials.
        /// </summary>
        [EditorDisplay("Geometry"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(60), DefaultValue(true)]
        public bool MergeMeshes { get; set; } = true;

        /// <summary>
        /// Enable/disable importing meshes Level of Details.
        /// </summary>
        [EditorDisplay("Geometry", "Import LODs"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(70), DefaultValue(true)]
        public bool ImportLODs { get; set; } = true;

        /// <summary>
        /// Enable/disable importing vertex colors (channel 0 only).
        /// </summary>
        [EditorDisplay("Geometry"), VisibleIf(nameof(ShowModel))]
        [EditorOrder(80), DefaultValue(true)]
        public bool ImportVertexColors { get; set; } = true;

        /// <summary>
        /// Enable/disable importing blend shapes (morph targets).
        /// </summary>
        [EditorDisplay("Geometry"), VisibleIf(nameof(ShowSkinnedModel))]
        [EditorOrder(85), DefaultValue(false)]
        public bool ImportBlendShapes { get; set; } = false;

        /// <summary>
        /// The lightmap UVs source.
        /// </summary>
        [EditorDisplay("Geometry", "Lightmap UVs Source"), VisibleIf(nameof(ShowModel))]
        [EditorOrder(90), DefaultValue(ModelLightmapUVsSource.Disable)]
        public ModelLightmapUVsSource LightmapUVsSource { get; set; } = ModelLightmapUVsSource.Disable;

        /// <summary>
        /// If specified, all meshes which name starts with this prefix will be imported as a separate collision data (excluded used for rendering).
        /// </summary>
        [EditorDisplay("Geometry"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(100), DefaultValue("")]
        public string CollisionMeshesPrefix { get; set; }

        /// <summary>
        /// Custom uniform import scale.
        /// </summary>
        [EditorOrder(500), DefaultValue(1.0f), EditorDisplay("Transform")]
        public float Scale { get; set; } = 1.0f;

        /// <summary>
        /// Custom import geometry rotation.
        /// </summary>
        [DefaultValue(typeof(Quaternion), "0,0,0,1")]
        [EditorOrder(510), EditorDisplay("Transform")]
        public Quaternion Rotation { get; set; } = Quaternion.Identity;

        /// <summary>
        /// Custom import geometry offset.
        /// </summary>
        [DefaultValue(typeof(Float3), "0,0,0")]
        [EditorOrder(520), EditorDisplay("Transform")]
        public Float3 Translation { get; set; } = Float3.Zero;

        /// <summary>
        /// If checked, the imported geometry will be shifted to the center of mass.
        /// </summary>
        [EditorOrder(530), DefaultValue(false), EditorDisplay("Transform")]
        public bool CenterGeometry { get; set; } = false;

        /// <summary>
        /// Imported animation duration mode. Can use the original value or overriden by settings.
        /// </summary>
        [EditorDisplay("Animation"), VisibleIf(nameof(ShowAnimation))]
        [EditorOrder(1000), DefaultValue(AnimationDuration.Imported)]
        public AnimationDuration Duration { get; set; } = AnimationDuration.Imported;

        /// <summary>
        /// Imported animation first frame index. Used only if Duration mode is set to Custom.
        /// </summary>
        [EditorDisplay("Animation"), VisibleIf(nameof(ShowFramesRange))]
        [EditorOrder(1010), DefaultValue(0.0f), Limit(0)]
        public float FramesRangeStart { get; set; } = 0;

        /// <summary>
        /// Imported animation last frame index. Used only if Duration mode is set to Custom.
        /// </summary>
        [EditorDisplay("Animation"), VisibleIf(nameof(ShowFramesRange))]
        [EditorOrder(1020), DefaultValue(0.0f), Limit(0)]
        public float FramesRangeEnd { get; set; } = 0;

        private bool ShowFramesRange => ShowAnimation && Duration == AnimationDuration.Custom;

        /// <summary>
        /// The imported animation default frame rate. Can specify the default frames per second amount for imported animation. If value is 0 then the original animation frame rate will be used.
        /// </summary>
        [EditorDisplay("Animation"), VisibleIf(nameof(ShowAnimation))]
        [EditorOrder(1025), DefaultValue(0.0f), Limit(0, 1000, 0.01f)]
        public float DefaultFrameRate { get; set; } = 0.0f;

        /// <summary>
        /// The imported animation sampling rate. If value is 0 then the original animation speed will be used.
        /// </summary>
        [EditorDisplay("Animation"), VisibleIf(nameof(ShowAnimation))]
        [EditorOrder(1030), DefaultValue(0.0f), Limit(0, 1000, 0.01f)]
        public float SamplingRate { get; set; } = 0.0f;
        
        /// <summary>
        /// The imported scale animation. If value is true then the scale animation will be imported.
        /// </summary>
        [EditorDisplay("Animation"), VisibleIf(nameof(ShowAnimation))]
        [EditorOrder(1035), DefaultValue(false)]
        public bool ImportScale { get; set; } = false;

        /// <summary>
        /// The imported animation will have removed tracks with no keyframes or unspecified data.
        /// </summary>
        [EditorDisplay("Animation"), VisibleIf(nameof(ShowAnimation))]
        [EditorOrder(1040), DefaultValue(true)]
        public bool SkipEmptyCurves { get; set; } = true;

        /// <summary>
        /// The imported animation channels will be optimized to remove redundant keyframes.
        /// </summary>
        [EditorDisplay("Animation"), VisibleIf(nameof(ShowAnimation))]
        [EditorOrder(1050), DefaultValue(true)]
        public bool OptimizeKeyframes { get; set; } = true;

        /// <summary>
        /// Enables root motion extraction support from this animation.
        /// </summary>
        [EditorDisplay("Animation"), VisibleIf(nameof(ShowAnimation))]
        [EditorOrder(1060), DefaultValue(false)]
        public bool EnableRootMotion { get; set; } = false;

        /// <summary>
        /// The custom node name to be used as a root motion source. If not specified the actual root node will be used.
        /// </summary>
        [EditorDisplay("Animation"), VisibleIf(nameof(ShowAnimation))]
        [EditorOrder(1070), DefaultValue(typeof(string), "")]
        public string RootNodeName { get; set; }

        /// <summary>
        /// If checked, the importer will generate a sequence of LODs based on the base LOD index.
        /// </summary>
        [EditorDisplay("Level Of Detail", "Generate LODs"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(1100), DefaultValue(false)]
        public bool GenerateLODs { get; set; } = false;

        /// <summary>
        /// The index of the LOD from the source model data to use as a reference for following LODs generation.
        /// </summary>
        [EditorDisplay("Level Of Detail", "Base LOD"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(1110), DefaultValue(0), Limit(0, Model.MaxLODs - 1)]
        public int BaseLOD { get; set; } = 0;

        /// <summary>
        /// The amount of LODs to include in the model (all remaining ones starting from Base LOD will be generated).
        /// </summary>
        [EditorDisplay("Level Of Detail", "LOD Count"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(1120), DefaultValue(4), Limit(1, Model.MaxLODs)]
        public int LODCount { get; set; } = 4;

        /// <summary>
        /// The target amount of triangles for the generated LOD (based on the higher LOD). Normalized to range 0-1. For instance 0.4 cuts the triangle count to 40%.
        /// </summary>
        [EditorDisplay("Level Of Detail"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(1130), DefaultValue(0.5f), Limit(0, 1, 0.001f)]
        public float TriangleReduction { get; set; } = 0.5f;

        /// <summary>
        /// If checked, the importer will create materials for model meshes as specified in the file.
        /// </summary>
        [EditorDisplay("Materials"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(400), DefaultValue(true)]
        public bool ImportMaterials { get; set; } = true;

        /// <summary>
        /// If checked, the importer will import texture files used by the model and any embedded texture resources.
        /// </summary>
        [EditorDisplay("Materials"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(410), DefaultValue(true)]
        public bool ImportTextures { get; set; } = true;

        /// <summary>
        /// If checked, the importer will try to restore the model material slots.
        /// </summary>
        [EditorDisplay("Materials", "Restore Materials On Reimport"), VisibleIf(nameof(ShowGeometry))]
        [EditorOrder(420), DefaultValue(true)]
        public bool RestoreMaterialsOnReimport { get; set; } = true;

        /// <summary>
        /// If checked, enables generation of Signed Distance Field (SDF).
        /// </summary>
        [EditorDisplay("SDF"), VisibleIf(nameof(ShowModel))]
        [EditorOrder(1500), DefaultValue(false)]
        public bool GenerateSDF { get; set; } = false;

        /// <summary>
        /// Resolution scale for generated Signed Distance Field (SDF) texture. Higher values improve accuracy but increase memory usage and reduce performance.
        /// </summary>
        [EditorDisplay("SDF"), VisibleIf(nameof(ShowModel))]
        [EditorOrder(1510), DefaultValue(1.0f), Limit(0.0001f, 100.0f)]
        public float SDFResolution { get; set; } = 1.0f;

        /// <summary>
        /// If checked, the imported mesh/animations are splitted into separate assets. Used if ObjectIndex is set to -1.
        /// </summary>
        [EditorOrder(2000), DefaultValue(false), EditorDisplay("Splitting")]
        public bool SplitObjects { get; set; } = false;

        /// <summary>
        /// The zero-based index for the mesh/animation clip to import. If the source file has more than one mesh/animation it can be used to pick a desire object. Default -1 imports all objects.
        /// </summary>
        [EditorOrder(2010), DefaultValue(-1), EditorDisplay("Splitting")]
        public int ObjectIndex { get; set; } = -1;

        private bool ShowGeometry => Type == ModelType.Model || Type == ModelType.SkinnedModel;
        private bool ShowModel => Type == ModelType.Model;
        private bool ShowSkinnedModel => Type == ModelType.SkinnedModel;
        private bool ShowAnimation => Type == ModelType.Animation;

        [StructLayout(LayoutKind.Sequential)]
        internal struct InternalOptions
        {
            public ModelType Type;

            // Geometry
            public byte CalculateNormals;
            public float SmoothingNormalsAngle;
            public byte FlipNormals;
            public float SmoothingTangentsAngle;
            public byte CalculateTangents;
            public byte OptimizeMeshes;
            public byte MergeMeshes;
            public byte ImportLODs;
            public byte ImportVertexColors;
            public byte ImportBlendShapes;
            public ModelLightmapUVsSource LightmapUVsSource;
            public string CollisionMeshesPrefix;

            // Transform
            public float Scale;
            public Quaternion Rotation;
            public Float3 Translation;
            public byte CenterGeometry;

            // Animation
            public AnimationDuration Duration;
            public float FramesRangeStart;
            public float FramesRangeEnd;
            public float DefaultFrameRate;
            public float SamplingRate;
            public byte ImportScale;
            public byte SkipEmptyCurves;
            public byte OptimizeKeyframes;
            public byte EnableRootMotion;
            public string RootNodeName;

            // Level Of Detail
            public byte GenerateLODs;
            public int BaseLOD;
            public int LODCount;
            public float TriangleReduction;

            // Misc
            public byte ImportMaterials;
            public byte ImportTextures;
            public byte RestoreMaterialsOnReimport;

            // SDF
            public byte GenerateSDF;
            public float SDFResolution;

            // Splitting
            public byte SplitObjects;
            public int ObjectIndex;
        }

        internal void ToInternal(out InternalOptions options)
        {
            options = new InternalOptions
            {
                Type = Type,
                CalculateNormals = (byte)(CalculateNormals ? 1 : 0),
                SmoothingNormalsAngle = SmoothingNormalsAngle,
                FlipNormals = (byte)(FlipNormals ? 1 : 0),
                SmoothingTangentsAngle = SmoothingTangentsAngle,
                CalculateTangents = (byte)(CalculateTangents ? 1 : 0),
                OptimizeMeshes = (byte)(OptimizeMeshes ? 1 : 0),
                MergeMeshes = (byte)(MergeMeshes ? 1 : 0),
                ImportLODs = (byte)(ImportLODs ? 1 : 0),
                ImportVertexColors = (byte)(ImportVertexColors ? 1 : 0),
                ImportBlendShapes = (byte)(ImportBlendShapes ? 1 : 0),
                LightmapUVsSource = LightmapUVsSource,
                CollisionMeshesPrefix = CollisionMeshesPrefix,
                Scale = Scale,
                Rotation = Rotation,
                Translation = Translation,
                CenterGeometry = (byte)(CenterGeometry ? 1 : 0),
                Duration = Duration,
                FramesRangeStart = FramesRangeStart,
                FramesRangeEnd = FramesRangeEnd,
                DefaultFrameRate = DefaultFrameRate,
                SamplingRate = SamplingRate,
                ImportScale = (byte)(ImportScale ? 1 : 0),
                SkipEmptyCurves = (byte)(SkipEmptyCurves ? 1 : 0),
                OptimizeKeyframes = (byte)(OptimizeKeyframes ? 1 : 0),
                EnableRootMotion = (byte)(EnableRootMotion ? 1 : 0),
                RootNodeName = RootNodeName,
                GenerateLODs = (byte)(GenerateLODs ? 1 : 0),
                BaseLOD = BaseLOD,
                LODCount = LODCount,
                TriangleReduction = TriangleReduction,
                ImportMaterials = (byte)(ImportMaterials ? 1 : 0),
                ImportTextures = (byte)(ImportTextures ? 1 : 0),
                RestoreMaterialsOnReimport = (byte)(RestoreMaterialsOnReimport ? 1 : 0),
                GenerateSDF = (byte)(GenerateSDF ? 1 : 0),
                SDFResolution = SDFResolution,
                SplitObjects = (byte)(SplitObjects ? 1 : 0),
                ObjectIndex = ObjectIndex,
            };
        }

        internal void FromInternal(ref InternalOptions options)
        {
            Type = options.Type;
            CalculateNormals = options.CalculateNormals != 0;
            SmoothingNormalsAngle = options.SmoothingNormalsAngle;
            FlipNormals = options.FlipNormals != 0;
            SmoothingTangentsAngle = options.SmoothingTangentsAngle;
            CalculateTangents = options.CalculateTangents != 0;
            OptimizeMeshes = options.OptimizeMeshes != 0;
            MergeMeshes = options.MergeMeshes != 0;
            ImportLODs = options.ImportLODs != 0;
            ImportVertexColors = options.ImportVertexColors != 0;
            ImportBlendShapes = options.ImportBlendShapes != 0;
            LightmapUVsSource = options.LightmapUVsSource;
            CollisionMeshesPrefix = options.CollisionMeshesPrefix;
            Scale = options.Scale;
            Rotation = options.Rotation;
            Translation = options.Translation;
            CenterGeometry = options.CenterGeometry != 0;
            FramesRangeStart = options.FramesRangeStart;
            FramesRangeEnd = options.FramesRangeEnd;
            DefaultFrameRate = options.DefaultFrameRate;
            SamplingRate = options.SamplingRate;
            ImportScale = options.ImportScale != 0;
            SkipEmptyCurves = options.SkipEmptyCurves != 0;
            OptimizeKeyframes = options.OptimizeKeyframes != 0;
            EnableRootMotion = options.EnableRootMotion != 0;
            RootNodeName = options.RootNodeName;
            GenerateLODs = options.GenerateLODs != 0;
            BaseLOD = options.BaseLOD;
            LODCount = options.LODCount;
            TriangleReduction = options.TriangleReduction;
            ImportMaterials = options.ImportMaterials != 0;
            ImportTextures = options.ImportTextures != 0;
            RestoreMaterialsOnReimport = options.RestoreMaterialsOnReimport != 0;
            GenerateSDF = options.GenerateSDF != 0;
            SDFResolution = options.SDFResolution;
            SplitObjects = options.SplitObjects != 0;
            ObjectIndex = options.ObjectIndex;
        }

        /// <summary>
        /// Tries the restore the asset import options from the target resource file. Applies the project default options too.
        /// </summary>
        /// <param name="options">The options.</param>
        /// <param name="assetPath">The asset path.</param>
        /// <returns>True settings has been restored, otherwise false.</returns>
        public static void TryRestore(ref ModelImportSettings options, string assetPath)
        {
            ModelImportEntry.Internal_GetModelImportOptions(assetPath, out var internalOptions);
            options.FromInternal(ref internalOptions);
        }
    }

    /// <summary>
    /// Model asset import entry.
    /// </summary>
    /// <seealso cref="AssetImportEntry" />
    public class ModelImportEntry : AssetImportEntry
    {
        private ModelImportSettings _settings = new ModelImportSettings();

        /// <summary>
        /// Initializes a new instance of the <see cref="ModelImportEntry"/> class.
        /// </summary>
        /// <param name="request">The import request.</param>
        public ModelImportEntry(ref Request request)
        : base(ref request)
        {
            // Try to restore target asset model import options (useful for fast reimport)
            ModelImportSettings.TryRestore(ref _settings, ResultUrl);
        }

        /// <inheritdoc />
        public override object Settings => _settings;

        /// <inheritdoc />
        public override bool TryOverrideSettings(object settings)
        {
            if (settings is ModelImportSettings o)
            {
                _settings = o;
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool Import()
        {
            return Editor.Import(SourceUrl, ResultUrl, _settings);
        }

        #region Internal Calls

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_GetModelImportOptions(string path, out ModelImportSettings.InternalOptions result);

        #endregion
    }
}
