// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Config/Settings.h"

/// <summary>
/// Network settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API NetworkSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkSettings);
    API_AUTO_SERIALIZATION();

public:
    /// <summary>
    /// Maximum amount of active network clients in a game session. Used by server or host to limit amount of players and spectators.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"General\")")
    int32 MaxClients = 100;

    /// <summary>
    /// Network protocol version of the game. Network clients and server can use only the same protocol version (verified upon client joining).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"General\")")
    uint32 ProtocolVersion = 1;

    /// <summary>
    /// The target amount of the network system updates per second. Higher values provide better network synchronization (eg. 60 for shooters), lower values reduce network usage and performance impact (eg. 30 for strategy games). Can be used to tweak networking performance impact on game. Cannot be higher that UpdateFPS (from Time Settings). Use 0 to run every game update.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), Limit(0, 1000), EditorDisplay(\"General\", \"Network FPS\")")
    float NetworkFPS = 60.0f;

    /// <summary>
    /// Address of the server (server/host always runs on localhost). Only IPv4 is supported.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1000), EditorDisplay(\"Transport\")")
    String Address = TEXT("127.0.0.1");

    /// <summary>
    /// The port for the network peer.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1010), EditorDisplay(\"Transport\")")
    uint16 Port = 7777;

    /// <summary>
    /// The type of the network driver (implements INetworkDriver) that will be used to create, manage, send and receive messages over the network.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1100), EditorDisplay(\"Transport\"), TypeReference(typeof(INetworkDriver)), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.TypeNameEditor\")")
    StringAnsi NetworkDriver = "FlaxEngine.Networking.ENetDriver";

public:
    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static NetworkSettings* Get();

    // [SettingsBase]
    void Apply() override;
};
