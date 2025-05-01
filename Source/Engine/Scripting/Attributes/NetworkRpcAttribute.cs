// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.Networking;

namespace FlaxEngine
{
    /// <summary>
    /// Indicates that a method is Remote Procedure Call which can be invoked on client and executed on server or invoked on server and executed on clients.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method)]
    public sealed class NetworkRpcAttribute : Attribute
    {
        /// <summary>
        /// True if RPC should be executed on server.
        /// </summary>
        public bool Server;

        /// <summary>
        /// True if RPC should be executed on client.
        /// </summary>
        public bool Client;

        /// <summary>
        /// Network channel using which RPC should be send.
        /// </summary>
        public NetworkChannelType Channel;

        /// <summary>
        /// Initializes a new instance of the <see cref="NetworkRpcAttribute"/> class.
        /// </summary>
        /// <param name="server">True if RPC should be executed on server.</param>
        /// <param name="client">True if RPC should be executed on client.</param>
        /// <param name="channel">Network channel using which RPC should be send.</param>
        public NetworkRpcAttribute(bool server = false, bool client = false, NetworkChannelType channel = NetworkChannelType.ReliableOrdered)
        {
            Server = server;
            Client = client;
            Channel = channel;
        }
    }
}
