// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    unsafe partial struct PixelFormatSampler
    {
        /// <summary>
        /// Element format.
        /// </summary>
        public PixelFormat Format;

        /// <summary>
        /// Element size in bytes.
        /// </summary>
        public int PixelSize;

        /// <summary>
        /// Read data function.
        /// </summary>
        public delegate* unmanaged<void*, Float4> Read;

        /// <summary>
        /// Write data function.
        /// </summary>
        public delegate* unmanaged<void*, ref Float4, void> Write;

        /// <summary>
        /// Tries to get a sampler tool for the specified format to read pixels.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="sampler">The sampler object or empty when cannot sample the given format.</param>
        /// <returns>True if got sampler, otherwise false.</returns>
        public static bool Get(PixelFormat format, out PixelFormatSampler sampler)
        {
            PixelFormatExtensions.Internal_GetSamplerInternal(format, out var pixelSize, out var read, out var write);
            sampler = new PixelFormatSampler
            {
                Format = format,
                PixelSize = pixelSize,
                Read = (delegate* unmanaged<void*, Float4>)read.ToPointer(),
                Write = (delegate* unmanaged<void*, ref Float4, void>)write.ToPointer(),
            };
            return pixelSize != 0;
        }
    }
}
