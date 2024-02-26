// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Builder.h"
#include "Engine/Utilities/RectPack.h"

#if COMPILE_WITH_GI_BAKING

namespace ShadowsOfMordor
{
    class AtlasChartsPacker
    {
    public:

        struct Node : RectPack<Node>
        {
            Builder::LightmapUVsChart* Chart = nullptr;

            Node(uint32 x, uint32 y, uint32 width, uint32 height)
                : RectPack<Node>(x, y, width, height)
            {
            }

            void OnInsert(Builder::LightmapUVsChart* chart, const LightmapSettings* settings)
            {
                Chart = chart;
                const float invSize = 1.0f / (int32)settings->AtlasSize;
                chart->Result.UVsArea = Rectangle(X * invSize, Y * invSize, chart->Width * invSize, chart->Height * invSize);
            }
        };

    private:

        Node _root;
        const LightmapSettings* _settings;

    public:

        /// <summary>
        /// Initializes a new instance of the <see cref="AtlasChartsPacker"/> class.
        /// </summary>
        /// <param name="settings">The settings.</param>
        AtlasChartsPacker(const LightmapSettings* settings)
            : _root(settings->ChartsPadding, settings->ChartsPadding, (int32)settings->AtlasSize - settings->ChartsPadding, (int32)settings->AtlasSize - settings->ChartsPadding)
            , _settings(settings)
        {
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="AtlasChartsPacker"/> class.
        /// </summary>
        ~AtlasChartsPacker()
        {
        }

    public:

        /// <summary>
        /// Inserts the specified chart into atlas .
        /// </summary>
        /// <param name="chart">The chart.</param>
        /// <returns></returns>
        Node* Insert(Builder::LightmapUVsChart* chart)
        {
            return _root.Insert(chart->Width, chart->Height, _settings->ChartsPadding, chart, _settings);
        }
    };
};

#endif
