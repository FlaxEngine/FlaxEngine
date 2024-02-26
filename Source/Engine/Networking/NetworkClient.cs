// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine.Networking
{
    partial class NetworkClient
    {
        /// <inheritdoc />
        public override string ToString()
        {
            return $"NetworkClient Id={ClientId}, ConnectionId={Connection.ConnectionId}";
        }
    }
}
