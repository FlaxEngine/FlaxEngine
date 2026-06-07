// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_RENDER_PERF
using FlaxEditor;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// Opens PerfSDK metrics window for a GPU profiler region.
    /// </summary>
    internal static class GpuProfilerPerfSdk
    {
        public static void TryOpen(string regionName, float gpuTimeMs, long drawCalls, long triangles, long vertices)
        {
            if (!RenderPerfTools.IsCompiled)
                return;

            if (!RenderPerfTools.TryOpenRegionProfiler(regionName, gpuTimeMs, (int)drawCalls, (int)triangles, (int)vertices))
            {
                string error = RenderPerfTools.ErrorText;
                if (!string.IsNullOrEmpty(error))
                    MessageBox.Show(error, "PerfSDK", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            Editor.Instance.Windows.PerfSdkMetricsWin.ShowRegion(regionName);
        }
    }
}
#endif
