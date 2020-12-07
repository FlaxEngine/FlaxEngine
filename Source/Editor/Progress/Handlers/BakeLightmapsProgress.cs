// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Progress.Handlers
{
    /// <summary>
    /// Static lightmaps baking progress reporting handler.
    /// </summary>
    /// <seealso cref="FlaxEditor.Progress.ProgressHandler" />
    public sealed class BakeLightmapsProgress : ProgressHandler
    {
        private static int _canBake;

        /// <summary>
        /// Gets a value indicating whether GPU lightmaps baking is supported on this device.
        /// </summary>
        public static bool CanBake
        {
            get
            {
                if (_canBake == 0)
                {
                    var instance = GPUDevice.Instance;
                    if (instance == null)
                        return false;
                    var limits = instance.Limits;
                    _canBake = limits.HasCompute && limits.MaximumTexture2DSize >= 8 * 1024 && instance.TotalGraphicsMemory >= 2 * 1024 ? 1 : 2;
                }
                return _canBake == 1;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="BakeLightmapsProgress"/> class.
        /// </summary>
        public BakeLightmapsProgress()
        {
            Editor.LightmapsBakeStart += OnLightmapsBakeStart;
            Editor.LightmapsBakeProgress += OnLightmapsBakeProgress;
            Editor.LightmapsBakeEnd += OnLightmapsBakeEnd;
        }

        private void OnLightmapsBakeStart()
        {
            OnStart();
        }

        private void OnLightmapsBakeProgress(Editor.LightmapsBakeSteps step, float stepProgress, float totalProgress)
        {
            OnUpdate(totalProgress, string.Format("Building lightmaps {0}% ...", Utils.RoundTo2DecimalPlaces(totalProgress * 100)));
        }

        private void OnLightmapsBakeEnd(bool failed)
        {
            OnEnd();
        }
    }
}
