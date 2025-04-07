// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/CSG/CSGMesh.h"
#include "Builder.Config.h"

#if COMPILE_WITH_GI_BAKING

#include "Engine/Graphics/RenderTask.h"
#include "Engine/Core/Singleton.h"

// Forward declarations
#if COMPILE_WITH_ASSETS_IMPORTER

namespace DirectX
{
    class ScratchImage;
};
#endif
class Actor;
class Terrain;
class Foliage;
class StaticModel;
class GPUPipelineState;

namespace ShadowsOfMordor
{
    /// <summary>
    /// Shadows Of Mordor lightmaps builder utility.
    /// </summary>
    class Builder : public Singleton<Builder>
    {
    public:

        enum class GeometryType
        {
            StaticModel,
            Terrain,
            Foliage,
        };

        struct LightmapUVsChart
        {
            int32 Width;
            int32 Height;

            LightmapEntry Result;

            int32 EntryIndex;
        };

        struct GeometryEntry
        {
            GeometryType Type;
            float Scale;
            BoundingBox Box;
            Rectangle UVsBox;

            union
            {
                struct
                {
                    StaticModel* Actor;
                } AsStaticModel;

                struct
                {
                    Terrain* Actor;
                    int32 PatchIndex;
                    int32 ChunkIndex;
                } AsTerrain;

                struct
                {
                    Foliage* Actor;
                    int32 InstanceIndex;
                    int32 TypeIndex;
                    int32 MeshIndex;
                } AsFoliage;
            };

            int32 ChartIndex;
        };

        typedef Array<GeometryEntry> GeometryEntriesCollection;
        typedef Array<LightmapUVsChart> LightmapUVsChartsCollection;

        enum BuildingStage
        {
            CleanLightmaps = 0,
            RenderCache,
            PostprocessCache,
            ClearLightmapData,
            RenderHemispheres,
            PostprocessLightmaps,
        };

        /// <summary>
        /// Single hemisphere data
        /// </summary>
        struct HemisphereData
        {
            Float3 Position;
            Float3 Normal;

            int16 TexelX;
            int16 TexelY;
        };

        /// <summary>
        /// Per lightmap cache data
        /// </summary>
        class LightmapBuildCache
        {
        public:

            Array<int32> Entries;
            Array<HemisphereData> Hemispheres;
            GPUBuffer* LightmapData = nullptr;
#if HEMISPHERES_BAKE_STATE_SAVE
            // Restored data for the lightmap from the loaded state (copied to the LightmapData on first hemispheres render job)
            Array<byte> LightmapDataInit;
#endif

            ~LightmapBuildCache();

            bool Init(const LightmapSettings* settings);
        };

        /// <summary>
        /// Per scene cache data
        /// </summary>
        class SceneBuildCache
        {
        public:

            // Meta
            Builder* Builder;
            int32 SceneIndex;

            // Data
            Scene* Scene;
            CriticalSection EntriesLocker;
            GeometryEntriesCollection Entries;
            LightmapUVsChartsCollection Charts;
            Array<LightmapBuildCache> Lightmaps;
            GPUBuffer* TempLightmapData;

            // Stats
            int32 LightmapsCount;
            int32 HemispheresCount;
            int32 MergedHemispheresCount;

        public:

            /// <summary>
            /// Initializes a new instance of the <see cref="SceneBuildCache"/> class.
            /// </summary>
            SceneBuildCache();

        public:

            /// <summary>
            /// Gets the lightmaps baking settings.
            /// </summary>
            /// <returns>Settings</returns>
            const LightmapSettings& GetSettings() const;

        public:

            /// <summary>
            /// Waits for lightmap textures being fully loaded before baking process.
            /// </summary>
            /// <returns>True if failed, otherwise false.</returns>
            bool WaitForLightmaps();

            /// <summary>
            /// Updates the lightmaps textures data.
            /// </summary>
            void UpdateLightmaps();

            /// <summary>
            /// Initializes this instance.
            /// </summary>
            /// <param name="builder">The builder.</param>
            /// <param name="index">The scene index.</param>
            /// <param name="scene">The scene.</param>
            /// <returns>True if failed, otherwise false.</returns>
            bool Init(ShadowsOfMordor::Builder* builder, int32 index, ::Scene* scene);

            /// <summary>
            /// Releases this scene data cache.
            /// </summary>
            void Release();

        public:

            // Importing lightmaps data
            BytesContainer ImportLightmapTextureData;
            int32 ImportLightmapIndex;
            int32 ImportLightmapTextureIndex;
#if COMPILE_WITH_ASSETS_IMPORTER
            bool onImportLightmap(TextureData& image);
#endif
        };

        class BuilderRenderTask : public SceneRenderTask
        {
            void OnRender(GPUContext* context) override
            {
                Builder::Instance()->onJobRender(context);
            }
        };

    private:

        friend class ShadowsOfMordorBuilderService;
        CriticalSection _locker;
        bool _wasBuildCalled;
        bool _isActive;
        volatile int64 _wasBuildCancelled;

        Array<SceneBuildCache*> _scenes;

        volatile int64 _wasJobDone;
        BuildingStage _stage;
        bool _wasStageDone;
        int32 _workerActiveSceneIndex;
        int32 _workerStagePosition0;
        int32 _workerStagePosition1;
        int32 _giBounceRunningIndex;
        uint64 _lastJobFrame;
        float _hemisphereTexelsTotalWeight;
        int32 _bounceCount;
        int32 _hemispheresPerJob;
        DateTime _hemispheresPerJobUpdateTime;
#if HEMISPHERES_BAKE_STATE_SAVE
        DateTime _lastStateSaveTime;
        bool _firstStateSave;
#endif
        BuildProgressStep _lastStep;
        DateTime _lastStepStart;

        BuilderRenderTask* _task = nullptr;
        GPUTexture* _output = nullptr;
        AssetReference<Shader> _shader;
        GPUPipelineState* _psRenderCacheModel = nullptr;
        GPUPipelineState* _psRenderCacheTerrain = nullptr;
        GPUPipelineState* _psBlurCache = nullptr;
        GPUBuffer* _irradianceReduction = nullptr;
        GPUTexture* _cachePositions = nullptr;
        GPUTexture* _cacheNormals = nullptr;

    public:

        Builder();

    public:

        /// <summary>
        /// Called on building start
        /// </summary>
        Action OnBuildStarted;

        /// <summary>
        /// Called on building progress made
        /// Arguments: current step, current step progress, total progress
        /// </summary>
        Delegate<BuildProgressStep, float, float> OnBuildProgress;

        /// <summary>
        /// Called on building finish (argument: true if build failed, otherwise false)
        /// </summary>
        Delegate<bool> OnBuildFinished;

    public:

        void CheckIfRestoreState();
        bool RestoreState();

        /// <summary>
        /// Starts building lightmap.
        /// </summary>
        void Build();

        /// <summary>
        /// Returns true if build is running.
        /// </summary>
        bool IsActive() const
        {
            return _isActive;
        }

        /// <summary>
        /// Sends cancel current build signal.
        /// </summary>
        void CancelBuild();

        void Dispose();

    private:

        void saveState();
        bool loadState();
        void deleteState();

        void reportProgress(BuildProgressStep step, float stepProgress);
        void reportProgress(BuildProgressStep step, float stepProgress, int32 subSteps);
        void reportProgress(BuildProgressStep step, float stepProgress, float totalProgress);
        void onJobRender(GPUContext* context);
        bool checkBuildCancelled();
        bool runStage(BuildingStage stage, bool resetPosition = true);
        bool initResources();
        void releaseResources();
        bool waitForJobDataSync();
        static bool sortCharts(const LightmapUVsChart& a, const LightmapUVsChart& b);

        bool doWorkInner(DateTime buildStart);
        int32 doWork();

        void cacheEntries();
        void generateCharts();
        void packCharts();
        void updateLightmaps();
        void updateEntries();
        void generateHemispheres();

#if DEBUG_EXPORT_LIGHTMAPS_PREVIEW
        static void exportLightmapPreview(SceneBuildCache* scene, int32 lightmapIndex);
#endif
#if DEBUG_EXPORT_CACHE_PREVIEW
        void exportCachePreview(SceneBuildCache* scene, GenerateHemispheresData& cacheData, LightmapBuildCache& lightmapEntry) const;
#endif
#if DEBUG_EXPORT_HEMISPHERES_PREVIEW
        void addDebugHemisphere(GPUContext* context, GPUTextureView* radianceMap);
        void downloadDebugHemisphereAtlases(SceneBuildCache* scene);
        void releaseDebugHemisphereAtlases();
#endif
    };
};

#endif
