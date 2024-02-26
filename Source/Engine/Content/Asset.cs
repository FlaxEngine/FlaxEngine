// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial class Asset
    {
        /// <inheritdoc />
        public override string ToString()
        {
            return $"{Path} ({GetType().Name})";
        }
    }
}
