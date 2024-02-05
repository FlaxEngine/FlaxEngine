// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Transform.h"

/// <summary>
/// The helper class for that handles active audio backend operations.
/// </summary>
class AudioBackendTools
{
public:
    struct Settings
    {
        float Volume = 1.0f;
        float DopplerFactor = 1.0f;
    };

    struct Listener
    {
        Vector3 Velocity;
        Vector3 Position;
        Quaternion Orientation;
    };

    struct Source
    {
        bool Is3D;
        float Volume;
        float Pitch;
        float Pan;
        float MinDistance;
        float Attenuation;
        float DopplerFactor;
        Vector3 Velocity;
        Vector3 Position;
        Quaternion Orientation;
    };

    enum class Channels
    {
        FrontLeft = 0,
        FrontRight = 1,
        Center = 2,
        BackLeft = 3,
        BackRight = 4,
        SideLeft = 5,
        SideRight = 6,
        MAX
    };

    struct SoundMix
    {
        float Pitch;
        float Volume;
        float Channels[(int32)Channels::MAX];
    };

    static SoundMix CalculateSoundMix(const Settings& settings, const Listener& listener, const Source& source, int32 channelCount = 2)
    {
        SoundMix mix;
        mix.Pitch = source.Pitch;
        mix.Volume = source.Volume * settings.Volume;
        if (source.Is3D)
        {
            const Transform listenerTransform(listener.Position, listener.Orientation);
            float distance = (float)Vector3::Distance(listener.Position, source.Position);
            float gain = 1;

            // Calculate attenuation (OpenAL formula for mode: AL_INVERSE_DISTANCE_CLAMPED)
            // [https://www.openal.org/documentation/openal-1.1-specification.pdf]
            distance = Math::Clamp(distance, source.MinDistance, MAX_float);
            const float dst = source.MinDistance + source.Attenuation * (distance - source.MinDistance);
            if (dst > 0)
                gain = source.MinDistance / dst;
            mix.Volume *= Math::Saturate(gain);

            // Calculate panning
            // Ramy Sadek and Chris Kyriakakis, 2004, "A Novel Multichannel Panning Method for Standard and Arbitrary Loudspeaker Configurations"
            // [https://www.researchgate.net/publication/235080603_A_Novel_Multichannel_Panning_Method_for_Standard_and_Arbitrary_Loudspeaker_Configurations]
            //TODO: hardcoded main speaker directions for 2, 3.1, 5.1 and 7.1 setups - these are simplified and could also be made configurable
            static const Float3 ChannelDirections[(int32)Channels::MAX] =
            {
                Float3(-1.0, 0.0, -1.0).GetNormalized(),
                Float3(1.0, 0.0, -1.0).GetNormalized(),
                Float3(0.0, 0.0, -1.0).GetNormalized(),
                Float3(-1.0, 0.0, 1.0).GetNormalized(),
                Float3(1.0, 0.0, 1.0).GetNormalized(),
                Float3(-1.0, 0.0, 0.0).GetNormalized(),
                Float3(1.0, 0.0, 0.0).GetNormalized(),
            };
            const Float3 sourceInListenerSpace = (Float3)listenerTransform.WorldToLocal(source.Position);
            const Float3 sourceToListener = Float3::Normalize(sourceInListenerSpace);
            float sqGainsSum = 0.0f;
            for (int32 i = 0; i < channelCount; i++)
            {
                float othersSum = 0.0f;
                for (int32 j = 0; j < channelCount; j++)
                    othersSum += (1.0f + Float3::Dot(ChannelDirections[i], ChannelDirections[j])) * 0.5f;
                const float sqGain = Math::Square(0.5f * Math::Pow(1.0f + Float3::Dot(ChannelDirections[i], sourceToListener), 2.0f) / othersSum);
                sqGainsSum += sqGain;
                mix.Channels[i] = sqGain;
            }
            for (int32 i = 0; i < channelCount; i++)
                mix.Channels[i] = Math::Sqrt(mix.Channels[i] / sqGainsSum);

            // Calculate doppler
            const Float3 velocityInListenerSpace = (Float3)listenerTransform.WorldToLocalVector(source.Velocity - listener.Velocity);
            const float velocity = velocityInListenerSpace.Length();
            const float dopplerFactor = settings.DopplerFactor * source.DopplerFactor;
            if (dopplerFactor > 0.0f && velocity > 0.0f)
            {
                constexpr float speedOfSound = 343.3f * 100.0f * 100.0f; // in air, in Flax units
                const float approachingFactor = Float3::Dot(sourceToListener, velocityInListenerSpace.GetNormalized());
                const float dopplerPitch = speedOfSound / (speedOfSound + velocity * approachingFactor);
                mix.Pitch *= Math::Clamp(dopplerPitch, 0.1f, 10.0f);
            }
        }
        else
        {
            mix.Channels[(int32)Channels::FrontLeft] = Math::Min(1.0f - source.Pan, 1.0f);
            mix.Channels[(int32)Channels::FrontRight] = Math::Min(1.0f + source.Pan, 1.0f);
            for (int32 i = 2; i < channelCount; i++)
                mix.Channels[i] = 0.0f;
        }
        return mix;
    }
};
