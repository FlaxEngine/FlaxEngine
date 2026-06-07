// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_RENDER_PERF
using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Editor window that shows live NVIDIA PerfSDK GPU metrics in a grouped table layout.
    /// </summary>
    public sealed class PerfSdkMetricsWindow : EditorWindow
    {
        private const float TopMargin = 8.0f;
        private const float StatusHeight = 18.0f;

        private Label _statusLabel;
        private ChipMetricsTable _table;

        /// <summary>
        /// Initializes a new instance of the <see cref="PerfSdkMetricsWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public PerfSdkMetricsWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Nvidia PerfSDK Metrics";
            Size = new Float2(430, 860);

            _statusLabel = new Label
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(8, TopMargin, 8, 0),
                Height = StatusHeight,
                HorizontalAlignment = TextAlignment.Near,
                IsScrollable = false,
                TextColor = Style.Current.Foreground * 0.75f,
                Parent = this,
            };

            _table = new ChipMetricsTable
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(8, TopMargin + StatusHeight + 4.0f, 8, 8),
                Parent = this,
            };
        }

        /// <summary>
        /// Shows metrics for the given GPU profiler region.
        /// </summary>
        /// <param name="regionName">The profiler region name.</param>
        public void ShowRegion(string regionName)
        {
            string apiName = FormatRendererType(GPUDevice.Instance != null ? GPUDevice.Instance.RendererType : RendererType.Unknown);
            Title = "Nvidia PerfSDK Metrics: " + regionName + " · " + apiName;
            UpdateView();
            FocusOrShow();
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            if (RenderPerfTools.IsSessionActive)
                UpdateView();
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            if (RenderPerfTools.IsSessionActive)
                RenderPerfTools.CloseRegionProfiler();
        }

        private void UpdateView()
        {
            _statusLabel.Text = RenderPerfTools.StatusText;
            _table.SetRegionStats(
                RenderPerfTools.RegionName,
                RenderPerfTools.RegionGpuTimeMs,
                RenderPerfTools.RegionDrawCalls,
                RenderPerfTools.RegionTriangles,
                RenderPerfTools.RegionVertices);
            _table.SetChipAreasText(RenderPerfTools.ChipAreasText);
        }

        private static string FormatRendererType(RendererType type)
        {
            switch (type)
            {
            case RendererType.DirectX10: return "DirectX 10";
            case RendererType.DirectX10_1: return "DirectX 10.1";
            case RendererType.DirectX11: return "DirectX 11";
            case RendererType.DirectX12: return "DirectX 12";
            case RendererType.Vulkan: return "Vulkan";
            case RendererType.OpenGL4_1: return "OpenGL 4.1";
            case RendererType.OpenGL4_4: return "OpenGL 4.4";
            case RendererType.OpenGLES3: return "OpenGL ES 3";
            case RendererType.OpenGLES3_1: return "OpenGL ES 3.1";
            case RendererType.Null: return "Null";
            case RendererType.WebGPU: return "WebGPU";
            default: return type.ToString();
            }
        }

        private sealed class ChipMetricsTable : ContainerControl
        {
            private const float RowHeight = 30.0f;
            private const float GroupHeaderHeight = 24.0f;
            private const float HeaderRowHeight = 22.0f;
            private const float FooterTopGap = 10.0f;
            private const float FooterLineHeight = 18.0f;
            private const float BarHeight = 14.0f;
            private const float ColumnGap = 6.0f;
            private const float GroupHeaderPadding = 8.0f;
            private const float AreaColumnRatio = 0.22f;
            private const float BarTrackRatio = 0.11f;
            private const float PercentLabelWidth = 40.0f;
            private const float BarPercentGap = 4.0f;

            private enum RowKind
            {
                GroupHeader,
                Metric,
            }

            private struct RowData
            {
                public RowKind Kind;
                public string Title;
                public float Target;
                public float Display;
                public string Detail;
                public bool HigherIsBetter;
            }

            private RowData[] _rows = Array.Empty<RowData>();
            private string _emptyMessage = "Collecting GPU samples...";
            private string _regionName = string.Empty;
            private string _regionStatsLine = string.Empty;

            public void SetRegionStats(string regionName, float gpuTimeMs, int drawCalls, int triangles, int vertices)
            {
                _regionName = regionName ?? string.Empty;
                _regionStatsLine = string.Format(
                    "GPU time: {0:0.000} ms   Draw calls: {1:#,0}   Triangles: {2:#,0}   Vertices: {3:#,0}",
                    gpuTimeMs,
                    drawCalls,
                    triangles,
                    vertices);
            }

            public void SetChipAreasText(string text)
            {
                if (string.IsNullOrEmpty(text))
                {
                    _rows = Array.Empty<RowData>();
                    return;
                }

                string[] lines = text.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
                var rows = new RowData[lines.Length];
                int count = 0;

                for (int i = 0; i < lines.Length; i++)
                {
                    string[] parts = lines[i].Split('\t');
                    if (parts.Length < 2)
                        continue;

                    if (parts[0] == "#GROUP")
                    {
                        rows[count++] = new RowData
                        {
                            Kind = RowKind.GroupHeader,
                            Title = parts[1],
                        };
                        continue;
                    }

                    if (parts.Length < 3 || !float.TryParse(parts[1], out float target))
                        continue;

                    bool higherIsBetter = parts.Length >= 4 && parts[3] == "1";
                    float display = target;
                    for (int j = 0; j < _rows.Length; j++)
                    {
                        if (_rows[j].Kind == RowKind.Metric && _rows[j].Title == parts[0])
                        {
                            display = _rows[j].Display;
                            break;
                        }
                    }

                    rows[count++] = new RowData
                    {
                        Kind = RowKind.Metric,
                        Title = parts[0],
                        Target = target,
                        Display = display,
                        Detail = parts[2],
                        HigherIsBetter = higherIsBetter,
                    };
                }

                if (count == lines.Length)
                    _rows = rows;
                else
                {
                    var trimmed = new RowData[count];
                    Array.Copy(rows, trimmed, count);
                    _rows = trimmed;
                }
            }

            /// <inheritdoc />
            public override void Update(float deltaTime)
            {
                if (Visible && _rows.Length > 0)
                {
                    float step = Mathf.Saturate(deltaTime * 6.0f);
                    for (int i = 0; i < _rows.Length; i++)
                    {
                        if (_rows[i].Kind != RowKind.Metric)
                            continue;

                        RowData row = _rows[i];
                        row.Display = Mathf.Lerp(row.Display, row.Target, step);
                        _rows[i] = row;
                    }
                }

                base.Update(deltaTime);
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                float contentWidth = Width;
                float areaWidth = contentWidth * AreaColumnRatio;
                float barTrackWidth = contentWidth * BarTrackRatio;
                float barWidth = barTrackWidth + PercentLabelWidth + BarPercentGap;
                float xBar = areaWidth + ColumnGap;
                float xDetail = xBar + barWidth + ColumnGap;
                float detailWidth = Math.Max(0.0f, contentWidth - xDetail);

                DrawHeader(style, areaWidth, xBar, barWidth, xDetail, detailWidth);

                if (_rows.Length == 0)
                {
                    var emptyRect = new Rectangle(0, HeaderRowHeight, contentWidth, RowHeight);
                    Render2D.DrawText(style.FontMedium, _emptyMessage, emptyRect, style.ForegroundGrey, TextAlignment.Near, TextAlignment.Center);
                    DrawFooter(style, HeaderRowHeight + RowHeight, contentWidth);
                }
                else
                {
                    float y = HeaderRowHeight;
                    for (int i = 0; i < _rows.Length; i++)
                    {
                        RowData row = _rows[i];
                        if (row.Kind == RowKind.GroupHeader)
                        {
                            DrawGroupHeader(style, row.Title, y, contentWidth);
                            y += GroupHeaderHeight;
                            continue;
                        }

                        if (IsMouseOver)
                        {
                            var hoverRect = new Rectangle(0, y, contentWidth, RowHeight);
                            Render2D.FillRectangle(hoverRect, style.BackgroundHighlighted * 0.35f);
                        }

                        var areaRect = new Rectangle(0, y, areaWidth, RowHeight);
                        Render2D.DrawText(style.FontMedium, row.Title, areaRect, style.Foreground, TextAlignment.Near, TextAlignment.Center);

                        DrawThroughputBar(style, xBar, y, barTrackWidth, row);

                        var detailRect = new Rectangle(xDetail, y, detailWidth, RowHeight);
                        Render2D.DrawText(style.FontMedium, row.Detail, detailRect, style.Foreground * 0.85f, TextAlignment.Near, TextAlignment.Center);
                        y += RowHeight;
                    }

                    DrawFooter(style, y, contentWidth);
                }
            }

            private static void DrawGroupHeader(Style style, string title, float y, float width)
            {
                var rect = new Rectangle(0, y, width, GroupHeaderHeight);
                Render2D.FillRectangle(rect, style.LightBackground * 1.15f);
                Render2D.DrawText(
                    style.FontMedium,
                    title,
                    new Rectangle(GroupHeaderPadding, y, width - GroupHeaderPadding * 2.0f, GroupHeaderHeight),
                    style.Foreground,
                    TextAlignment.Near,
                    TextAlignment.Center);
            }

            private void DrawFooter(Style style, float contentBottom, float contentWidth)
            {
                if (string.IsNullOrEmpty(_regionName) && string.IsNullOrEmpty(_regionStatsLine))
                    return;

                float footerY = contentBottom + FooterTopGap;
                var separatorRect = new Rectangle(0, footerY - 4.0f, contentWidth, 1.0f);
                Render2D.FillRectangle(separatorRect, style.Foreground * 0.12f);

                var regionRect = new Rectangle(0, footerY, contentWidth, FooterLineHeight);
                Render2D.DrawText(style.FontMedium, _regionName, regionRect, style.Foreground, TextAlignment.Near, TextAlignment.Center);

                var statsRect = new Rectangle(0, footerY + FooterLineHeight, contentWidth, FooterLineHeight);
                Render2D.DrawText(style.FontMedium, _regionStatsLine, statsRect, style.Foreground * 0.85f, TextAlignment.Near, TextAlignment.Center);
            }

            private static void DrawHeader(Style style, float areaWidth, float xBar, float barWidth, float xDetail, float detailWidth)
            {
                var headerBg = new Rectangle(0, 0, xDetail + detailWidth, HeaderRowHeight);
                Render2D.FillRectangle(headerBg, style.LightBackground);

                Render2D.DrawText(style.FontMedium, "Area", new Rectangle(0, 0, areaWidth, HeaderRowHeight), style.Foreground, TextAlignment.Near, TextAlignment.Center);
                Render2D.DrawText(style.FontMedium, "Throughput", new Rectangle(xBar, 0, barWidth, HeaderRowHeight), style.Foreground, TextAlignment.Near, TextAlignment.Center);
                Render2D.DrawText(style.FontMedium, "Metrics", new Rectangle(xDetail, 0, detailWidth, HeaderRowHeight), style.Foreground, TextAlignment.Near, TextAlignment.Center);
            }

            private static void DrawThroughputBar(Style style, float x, float y, float barTrackWidth, RowData row)
            {
                float barY = y + (RowHeight - BarHeight) * 0.5f;
                var bgRect = new Rectangle(x, barY, barTrackWidth, BarHeight);
                Render2D.FillRectangle(bgRect, style.Background * 1.2f);

                float fill = Mathf.Clamp(row.Display, 0.0f, 100.0f) * 0.01f;
                if (fill > 0.001f)
                {
                    var fillRect = new Rectangle(x, barY, barTrackWidth * fill, BarHeight);
                    Render2D.FillRectangle(fillRect, GetBarColor(fill * 100.0f, row.HigherIsBetter, style));
                }

                Render2D.DrawRectangle(bgRect, style.Foreground * 0.15f);

                var percentRect = new Rectangle(x + barTrackWidth + BarPercentGap, y, PercentLabelWidth, RowHeight);
                Render2D.DrawText(style.FontMedium, Mathf.RoundToInt(row.Display) + "%", percentRect, style.Foreground, TextAlignment.Far, TextAlignment.Center);
            }

            private static Color GetBarColor(float value, bool higherIsBetter, Style style)
            {
                if (higherIsBetter)
                {
                    if (value >= 75.0f)
                        return style.ProgressNormal;
                    if (value >= 55.0f)
                        return Color.Lerp(style.ProgressNormal, Color.Yellow, 0.45f);
                    return Color.Lerp(Color.OrangeRed, Color.Yellow, 0.35f);
                }

                if (value >= 90.0f)
                    return Color.Lerp(Color.OrangeRed, style.ProgressNormal, 0.25f);
                if (value >= 70.0f)
                    return Color.Lerp(Color.Yellow, style.ProgressNormal, 0.35f);
                return style.ProgressNormal;
            }
        }
    }
}
#endif
