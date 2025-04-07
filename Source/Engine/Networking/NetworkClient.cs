// Copyright (c) Wojciech Figat. All rights reserved.

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
