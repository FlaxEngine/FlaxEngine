// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Online.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/DateTime.h"

/// <summary>
/// Online platform user presence common states.
/// </summary>
API_ENUM(Namespace="FlaxEngine.Online") enum class OnlinePresenceStates
{
    /// <summary>
    /// User is offline.
    /// </summary>
    Offline = 0,

    /// <summary>
    /// User is online.
    /// </summary>
    Online,

    /// <summary>
    /// User is online but busy.
    /// </summary>
    Busy,

    /// <summary>
    /// User is online but away (no activity for some time).
    /// </summary>
    Away,
};

/// <summary>
/// Online platform user description.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Online") struct FLAXENGINE_API OnlineUser
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(OnlineUser);

    /// <summary>
    /// Unique player identifier. Specific for a certain online platform.
    /// </summary>
    API_FIELD() Guid Id;

    /// <summary>
    /// The player name.
    /// </summary>
    API_FIELD() String Name;

    /// <summary>
    /// The current player presence state.
    /// </summary>
    API_FIELD() OnlinePresenceStates PresenceState;
};

/// <summary>
/// Online platform achievement description.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Online") struct FLAXENGINE_API OnlineAchievement
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(OnlineAchievement);

    /// <summary>
    /// Unique achievement identifier. Specific for a certain online platform.
    /// </summary>
    API_FIELD() String Identifier;

    /// <summary>
    /// Achievement name. Specific for a game.
    /// </summary>
    API_FIELD() String Name;

    /// <summary>
    /// The achievement title text.
    /// </summary>
    API_FIELD() String Title;

    /// <summary>
    /// The achievement description text.
    /// </summary>
    API_FIELD() String Description;

    /// <summary>
    /// True if achievement is hidden from user (eg. can see it once it's unlocked).
    /// </summary>
    API_FIELD() bool IsHidden = false;

    /// <summary>
    /// Achievement unlock percentage progress (normalized to 0-100 range).
    /// </summary>
    API_FIELD() float Progress = 0.0f;

    /// <summary>
    /// Date and time at which player unlocked the achievement.
    /// </summary>
    API_FIELD() DateTime UnlockTime = DateTime::MinValue();
};

/// <summary>
/// Interface for online platform providers for communicating with various multiplayer services such as player info, achievements, game lobby or in-game store.
/// </summary>
API_INTERFACE(Namespace="FlaxEngine.Online") class FLAXENGINE_API IOnlinePlatform
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(IOnlinePlatform);

    /// <summary>
    /// Finalizes an instance of the <see cref="IOnlinePlatform"/> class.
    /// </summary>
    virtual ~IOnlinePlatform() = default;

    /// <summary>
    /// Initializes the online platform services.
    /// </summary>
    /// <remarks>Called only by Online system.</remarks>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool Initialize() = 0;

    /// <summary>
    /// Shutdowns the online platform services.
    /// </summary>
    /// <remarks>Called only by Online system. Can be used to destroy the object.</remarks>
    API_FUNCTION() virtual void Deinitialize() = 0;

public:
    /// <summary>
    /// Logins the local user into the online platform.
    /// </summary>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool UserLogin(User* localUser = nullptr) = 0;

    /// <summary>
    /// Logout the local user from the online platform.
    /// </summary>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool UserLogout(User* localUser = nullptr) = 0;

    /// <summary>
    /// Checks if the local user is logged in.
    /// </summary>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if user is logged, otherwise false.</returns>
    API_FUNCTION() virtual bool GetUserLoggedIn(User* localUser = nullptr) = 0;

    /// <summary>
    /// Gets the player from the online platform.
    /// </summary>
    /// <param name="user">The local player user info.</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetUser(API_PARAM(Out) OnlineUser& user, User* localUser = nullptr) = 0;

    /// <summary>
    /// Gets the list of friends of the user from the online platform.
    /// </summary>
    /// <param name="friends">The result local player friends user infos.</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetFriends(API_PARAM(Out) Array<OnlineUser, HeapAllocation>& friends, User* localUser = nullptr) = 0;

public:
    /// <summary>
    /// Gets the list of all achievements for this game.
    /// </summary>
    /// <param name="achievements">The result achievements list</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetAchievements(API_PARAM(Out) Array<OnlineAchievement, HeapAllocation>& achievements, User* localUser = nullptr) = 0;

    /// <summary>
    /// Unlocks the achievement.
    /// </summary>
    /// <param name="name">The achievement name. Specific for a game.</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool UnlockAchievement(const StringView& name, User* localUser = nullptr) = 0;

    /// <summary>
    /// Updates the achievement unlocking progress (in range 0-100).
    /// </summary>
    /// <param name="name">The achievement name. Specific for a game.</param>
    /// <param name="progress">The achievement unlock progress (in range 0-100).</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool UnlockAchievementProgress(const StringView& name, float progress, User* localUser = nullptr) = 0;

#if !BUILD_RELEASE
    /// <summary>
    /// Resets the all achievements progress for this game.
    /// </summary>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool ResetAchievements(User* localUser = nullptr) = 0;
#endif

public:
    /// <summary>
    /// Gets the online statistical value.
    /// </summary>
    /// <param name="name">The stat name.</param>
    /// <param name="value">The result value.</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetStat(const StringView& name, API_PARAM(Out) float& value, User* localUser = nullptr) = 0;

    /// <summary>
    /// Sets the online statistical value.
    /// </summary>
    /// <param name="name">The stat name.</param>
    /// <param name="value">The value.</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool SetStat(const StringView& name, float value, User* localUser = nullptr) = 0;

public:
    /// <summary>
    /// Gets the online savegame data. Returns empty if savegame slot is unused.
    /// </summary>
    /// <param name="name">The savegame slot name.</param>
    /// <param name="data">The result data. Empty or null for unused slot name.</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetSaveGame(const StringView& name, API_PARAM(Out) Array<byte, HeapAllocation>& data, User* localUser = nullptr) = 0;

    /// <summary>
    /// Sets the online savegame data.
    /// </summary>
    /// <param name="name">The savegame slot name.</param>
    /// <param name="data">The data. Empty or null to delete slot (or mark as unused).</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool SetSaveGame(const StringView& name, const Span<byte>& data, User* localUser = nullptr) = 0;
};
