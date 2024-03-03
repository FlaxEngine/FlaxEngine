// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial struct AudioDataInfo
    {
        /// <summary>
        /// Gets the length of the audio data (in seconds).
        /// </summary>
        public float Length => (float)NumSamples / (float)Mathf.Max(1U, SampleRate * NumChannels);
    }
}
