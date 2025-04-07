// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Threading.Tasks;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Audio clip PCM data editor preview.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class AudioClipPreview : ContainerControl
    {
        /// <summary>
        /// The audio clip drawing modes.
        /// </summary>
        public enum DrawModes
        {
            /// <summary>
            /// Fills the whole control area with the full clip duration.
            /// </summary>
            Fill,

            /// <summary>
            /// Draws single audio clip. Uses the view scale parameter.
            /// </summary>
            Single,

            /// <summary>
            /// Draws the looped audio clip. Uses the view scale parameter.
            /// </summary>
            Looped,
        };

        private readonly object _locker = new object();
        private AudioClip _asset;
        private int _pcmSequence = 0;
        private float[] _pcmData;
        private AudioDataInfo _pcmInfo;

        /// <summary>
        /// Gets or sets the clip to preview.
        /// </summary>
        public AudioClip Asset
        {
            get => _asset;
            set
            {
                lock (_locker)
                {
                    if (_asset == value)
                        return;

                    _asset = value;
                    _pcmData = null;

                    if (_asset)
                    {
                        // Use async task to gather PCM data (engine loads it from the asset)
                        Task.Run(DownloadData);
                    }
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether audio data has been fetched from the asset (done as an async task). It is required to be valid in order to draw the audio buffer preview.
        /// </summary>
        public bool HasData
        {
            get
            {
                lock (_locker)
                {
                    return _pcmData != null;
                }
            }
        }

        /// <summary>
        /// Gets the cached audio data info.
        /// </summary>
        public AudioDataInfo DataInfo
        {
            get
            {
                lock (_locker)
                {
                    return _pcmInfo;
                }
            }
        }

        /// <summary>
        /// The draw mode.
        /// </summary>
        public DrawModes DrawMode = DrawModes.Fill;

        /// <summary>
        /// The view offset parameter. Shifts the audio preview (in seconds).
        /// </summary>
        public float ViewOffset = 0.0f;

        /// <summary>
        /// The view scale parameter. Increase it to zoom in the audio. Usage depends on the current <see cref="DrawMode"/>.
        /// </summary>
        public float ViewScale = 1.0f;

        /// <summary>
        /// The color of the audio PCM data spectrum.
        /// </summary>
        public Color Color = Color.White;

        /// <summary>
        /// The audio units per second (on time axis).
        /// </summary>
        public static readonly float UnitsPerSecond = 100.0f;

        /// <summary>
        /// Invalidates the cached audio PCM data and fetches it again from the asset.
        /// </summary>
        public void RefreshPreview()
        {
            lock (_locker)
            {
                if (_asset != null)
                {
                    // Release any cached data
                    _pcmData = null;

                    // Invalidate any in-flight data download to reject cached data due to refresh
                    if (_pcmSequence != 0)
                        _pcmSequence++;

                    // Use async task to gather PCM data (engine loads it from the asset)
                    Task.Run(DownloadData);
                }
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            lock (_locker)
            {
                var info = _pcmInfo;
                if (_asset == null || _pcmData == null || info.NumSamples == 0)
                    return;
                var height = Height;
                var width = Width;
                var samplesPerChannel = info.NumSamples / info.NumChannels;
                var length = (float)samplesPerChannel / info.SampleRate;
                var color = Color;
                if (!EnabledInHierarchy)
                    color *= 0.4f;
                var sampleValueScale = height / info.NumChannels;

                // Calculate the amount of samples that are contained in the view
                float unitsPerSecond = UnitsPerSecond * ViewScale;
                float clipDefaultWidth = length * unitsPerSecond;
                float clipsInView = width / clipDefaultWidth;
                float clipWidth;
                uint samplesPerIndex;
                switch (DrawMode)
                {
                case DrawModes.Fill:
                    clipsInView = 1.0f;
                    clipWidth = width;
                    samplesPerIndex = (uint)(samplesPerChannel / width) * info.NumChannels;
                    break;
                case DrawModes.Single:
                    clipsInView = Mathf.Min(clipsInView, 1.0f);
                    clipWidth = clipDefaultWidth;
                    samplesPerIndex = (uint)(info.SampleRate / unitsPerSecond) * info.NumChannels;
                    break;
                case DrawModes.Looped:
                    clipWidth = width / clipsInView;
                    samplesPerIndex = (uint)(info.SampleRate / unitsPerSecond) * info.NumChannels;
                    break;
                default: throw new ArgumentOutOfRangeException();
                }
                samplesPerIndex = Math.Max(samplesPerIndex, info.NumChannels);
                const uint maxSamplesPerIndex = 64;
                uint samplesPerIndexDiff = Math.Max(1, samplesPerIndex / Math.Min(samplesPerIndex, maxSamplesPerIndex));

                // Calculate the clip range in the view to optimize drawing (eg. if only part fo the clip is visible)
                Render2D.PeekClip(out var globalClipping);
                Render2D.PeekTransform(out var globalTransform);
                var globalRect = new Rectangle(globalTransform.M31, globalTransform.M32, width * globalTransform.M11, height * globalTransform.M22);
                var globalMask = Rectangle.Shared(globalClipping, globalRect);
                var globalTransformInv = Matrix3x3.Invert(globalTransform);
                var localRect = Rectangle.FromPoints(Matrix3x3.Transform2D(globalMask.UpperLeft, globalTransformInv), Matrix3x3.Transform2D(globalMask.BottomRight, globalTransformInv));
                var localRectMin = localRect.UpperLeft;
                var localRectMax = localRect.BottomRight;

                // Render each clip separately
                var viewOffset = (uint)(info.SampleRate * info.NumChannels * ViewOffset);
                for (uint clipIndex = 0; clipIndex < Mathf.CeilToInt(clipsInView); clipIndex++)
                {
                    var clipStart = clipWidth * clipIndex;
                    var clipEnd = clipStart + clipWidth;
                    var xStart = Mathf.Max(clipStart, localRectMin.X);
                    var xEnd = Mathf.Min(Mathf.Min(width, clipEnd), localRectMax.X);
                    var samplesOffset = (uint)((xStart - clipStart) * samplesPerIndex) + viewOffset;

                    // Render every audio channel separately
                    for (uint channelIndex = 0; channelIndex < info.NumChannels; channelIndex++)
                    {
                        uint currentSample = channelIndex + samplesOffset;
                        float yCenter = Y + ((2 * channelIndex) + 1) * height / (2.0f * info.NumChannels);
                        for (float pixelX = xStart; pixelX < xEnd; pixelX++)
                        {
                            float samplesSum = 0;
                            int samplesInPixel = 0;

                            uint samplesEnd = Math.Min(currentSample + samplesPerIndex, info.NumSamples);
                            for (uint sampleIndex = currentSample; sampleIndex < samplesEnd; sampleIndex += samplesPerIndexDiff)
                            {
                                samplesSum += Mathf.Abs(_pcmData[sampleIndex]);
                                samplesInPixel++;
                            }
                            currentSample = samplesEnd;

                            if (samplesInPixel > 0)
                            {
                                float sampleValueAvg = samplesSum / samplesInPixel;
                                float sampleValueAvgScaled = sampleValueAvg * sampleValueScale;
                                if (sampleValueAvgScaled > 0.1f)
                                {
                                    Render2D.DrawLine(new Float2(pixelX, yCenter - sampleValueAvgScaled), new Float2(pixelX, yCenter + sampleValueAvgScaled), color);
                                }
                            }
                        }
                    }
                }
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            lock (_locker)
            {
                _asset = null;
                _pcmData = null;
            }

            base.OnDestroy();
        }

        /// <summary>
        /// Downloads the audio clip raw PCM data. Use it from async thread to prevent blocking,
        /// </summary>
        private void DownloadData()
        {
            AudioClip asset;
            int sequence;
            lock (_locker)
            {
                asset = _asset;
                sequence = _pcmSequence;
            }
            if (!asset)
                return;

            float[] data;
            AudioDataInfo dataInfo;
            try
            {
                asset.ExtractDataFloat(out data, out dataInfo);
            }
            catch (Exception ex)
            {
                Editor.LogWarning("Failed to get audio clip PCM data. " + ex.Message);
                Editor.LogWarning(ex);
                return;
            }

            if (data.Length != dataInfo.NumSamples)
            {
                Editor.LogWarning("Failed to get audio clip PCM data. Invalid samples count. Returned buffer has other size.");
            }

            lock (_locker)
            {
                // If asset has been modified during data fetching, ignore it
                if (_asset == asset && _pcmSequence == sequence)
                {
                    _pcmSequence++;
                    _pcmData = data;
                    _pcmInfo = dataInfo;
                }
            }
        }
    }
}
