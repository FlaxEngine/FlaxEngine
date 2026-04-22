// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Materials/MaterialInfo.h"

template<typename ModelType, typename DrawInfoType>
FORCE_INLINE bool ModelDrawTransition(ModelType* model, const DrawInfoType& info)
{
    for (auto& e : *info.Buffer)
    {
        if (e.Material && EnumHasAllFlags(e.Material->GetInfo().FeaturesFlags, MaterialFeaturesFlags::DitheredLODTransition))
            return true;
    }
    for (auto& e : model->MaterialSlots)
    {
        if (e.Material && EnumHasAllFlags(e.Material->GetInfo().FeaturesFlags, MaterialFeaturesFlags::DitheredLODTransition))
            return true;
    }
    return false;
}

template<typename ModelType, typename DrawInfoType, typename ContextType>
FORCE_INLINE void ModelDraw(ModelType* model, const RenderContext& renderContext, const ContextType& context, const DrawInfoType& info)
{
    ASSERT(info.Buffer);
    if (!model->CanBeRendered())
        return;
    if (!info.Buffer->IsValidFor(model))
        info.Buffer->Setup(model);
    const auto frame = Engine::FrameCount;
    const auto modelFrame = info.DrawState->PrevFrame + 1;

    // Select a proper LOD index (model may be culled)
    int32 lodIndex;
    if (info.ForcedLOD != -1)
    {
        lodIndex = info.ForcedLOD;
    }
    else
    {
        lodIndex = RenderTools::ComputeModelLOD(model, info.Bounds.Center, (float)info.Bounds.Radius, renderContext);
        if (lodIndex == -1)
        {
            // Handling model fade-out transition
            if (modelFrame == frame && info.DrawState->PrevLOD != -1 && !renderContext.View.IsSingleFrame && ModelDrawTransition(model, info))
            {
                // Check if start transition
                if (info.DrawState->LODTransition == 255)
                {
                    info.DrawState->LODTransition = 0;
                }

                RenderTools::UpdateModelLODTransition(info.DrawState->LODTransition);

                // Check if end transition
                if (info.DrawState->LODTransition == 255)
                {
                    info.DrawState->PrevLOD = lodIndex;
                }
                else
                {
                    const auto prevLOD = model->ClampLODIndex(info.DrawState->PrevLOD);
                    const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
                    model->LODs.Get()[prevLOD].Draw(renderContext, info, normalizedProgress);
                }
            }

            return;
        }
    }
    lodIndex += info.LODBias + renderContext.View.ModelLODBias;
    lodIndex = model->ClampLODIndex(lodIndex);

    if (renderContext.View.IsSingleFrame)
    {
    }
    // Check if it's the new frame and could update the drawing state (note: model instance could be rendered many times per frame to different viewports)
    else if (modelFrame == frame)
    {
        // Check if materials use transition
        if (!ModelDrawTransition(model, info))
        {
            info.DrawState->PrevLOD = lodIndex;
            info.DrawState->LODTransition = 255;
        }

        // Check if start transition
        if (info.DrawState->PrevLOD != lodIndex && info.DrawState->LODTransition == 255)
        {
            info.DrawState->LODTransition = 0;
        }

        RenderTools::UpdateModelLODTransition(info.DrawState->LODTransition);

        // Check if end transition
        if (info.DrawState->LODTransition == 255)
        {
            info.DrawState->PrevLOD = lodIndex;
        }
    }
    // Check if there was a gap between frames in drawing this model instance
    else if (modelFrame < frame || info.DrawState->PrevLOD == -1)
    {
        // Reset state
        info.DrawState->PrevLOD = lodIndex;
        info.DrawState->LODTransition = 255;
    }

    // Draw
    if (info.DrawState->PrevLOD == lodIndex || info.DrawState->LODTransition == 255 || renderContext.View.IsSingleFrame)
    {
        model->LODs.Get()[lodIndex].Draw(context, info, 0.0f);
    }
    else if (info.DrawState->PrevLOD == -1)
    {
        const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
        model->LODs.Get()[lodIndex].Draw(context, info, 1.0f - normalizedProgress);
    }
    else
    {
        const auto prevLOD = model->ClampLODIndex(info.DrawState->PrevLOD);
        const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
        model->LODs.Get()[prevLOD].Draw(context, info, normalizedProgress);
        model->LODs.Get()[lodIndex].Draw(context, info, normalizedProgress - 1.0f);
    }
}
