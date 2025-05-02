// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Builder.h"
#include "Engine/Utilities/RectPack.h"

#if COMPILE_WITH_GI_BAKING

namespace ShadowsOfMordor
{
    class AtlasChartsPacker
    {
    public:

        struct Node : RectPackNode<int32>
        {
            Builder::LightmapUVsChart* Chart = nullptr;

            Node(Size x, Size y, Size width, Size height)
                : RectPackNode(x, y, width, height)
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

        RectPackAtlas<Node> _root;
        const LightmapSettings* _settings;

    public:

        /// <summary>
        /// Initializes a new instance of the <see cref="AtlasChartsPacker"/> class.
        /// </summary>
        /// <param name="settings">The settings.</param>
        AtlasChartsPacker(const LightmapSettings* settings)
            : _settings(settings)
        {
            _root.Init((int32)settings->AtlasSize, (int32)settings->AtlasSize, settings->ChartsPadding);
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
            return _root.Insert(chart->Width, chart->Height, chart, _settings);
        }
    };
};

#endif
