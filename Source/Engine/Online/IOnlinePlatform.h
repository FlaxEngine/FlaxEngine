// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Online.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/DateTime.h"

class Texture;

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
API_STRUCT(Namespace="FlaxEngine.Online", NoDefault) struct FLAXENGINE_API OnlineUser
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
    /// The current player rich-presence status text.
    /// </summary>
    API_FIELD() String PresenceStatus;

    /// <summary>
    /// The current player presence state.
    /// </summary>
    API_FIELD() OnlinePresenceStates PresenceState = OnlinePresenceStates::Online;

    /// <summary>
    /// The current game identifier that the user is playing. Empty if not playing any game or not supported by the platform. Can be used to check if the user is playing this game (IOnlinePlatform.GameId) or a different one.
    /// </summary>
    API_FIELD() Guid GameId = Guid::Empty;
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
/// Online platform leaderboards sorting modes.
/// </summary>
API_ENUM(Namespace="FlaxEngine.Online") enum class OnlineLeaderboardSortModes
{
    /// <summary>
    /// Don't sort stats.
    /// </summary>
    None = 0,

    /// <summary>
    /// Sort ascending, top-score is the lowest number (lower value is better).
    /// </summary>
    Ascending,

    /// <summary>
    /// Sort descending, top-score is the highest number (higher value is better).
    /// </summary>
    Descending,
};

/// <summary>
/// Online platform leaderboards display modes. Defines how leaderboard value should be used.
/// </summary>
API_ENUM(Namespace="FlaxEngine.Online") enum class OnlineLeaderboardValueFormats
{
    /// <summary>
    /// Undefined format.
    /// </summary>
    Undefined = 0,

    /// <summary>
    /// Raw numerical score.
    /// </summary>
    Numeric,

    /// <summary>
    /// Time in seconds.
    /// </summary>
    Seconds,

    /// <summary>
    /// Time in milliseconds.
    /// </summary>
    Milliseconds,
};

/// <summary>
/// Online platform leaderboard description.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Online") struct FLAXENGINE_API OnlineLeaderboard
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(OnlineLeaderboard);

    /// <summary>
    /// Unique leaderboard identifier. Specific for a certain online platform.
    /// </summary>
    API_FIELD() String Identifier;

    /// <summary>
    /// Leaderboard name. Specific for a game.
    /// </summary>
    API_FIELD() String Name;

    /// <summary>
    /// The leaderboard sorting method.
    /// </summary>
    API_FIELD() OnlineLeaderboardSortModes SortMode;

    /// <summary>
    /// The leaderboard values formatting.
    /// </summary>
    API_FIELD() OnlineLeaderboardValueFormats ValueFormat;

    /// <summary>
    /// The leaderboard rows count (amount of entries to access).
    /// </summary>
    API_FIELD() int32 EntriesCount;
};

/// <summary>
/// Online platform leaderboard entry description.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Online") struct FLAXENGINE_API OnlineLeaderboardEntry
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(OnlineLeaderboardEntry);

    /// <summary>
    /// The player who holds the entry.
    /// </summary>
    API_FIELD() OnlineUser User;

    /// <summary>
    /// The entry rank. Placement of the entry in the leaderboard (starts at 1 for the top-score).
    /// </summary>
    API_FIELD() int32 Rank;

    /// <summary>
    /// The entry score set in the leaderboard.
    /// </summary>
    API_FIELD() int32 Score;
};

/// <summary>
/// Online platform game overlay dialog types.
/// </summary>
API_ENUM(Namespace="FlaxEngine.Online") enum class OnlineOverlayDialog
{
    /// <summary>
    /// Displays the user login page.
    /// </summary>
    Login = 0,

    /// <summary>
    /// Displays the uweb browser.
    /// </summary>
    Website,

    /// <summary>
    /// Displays the user profile page.
    /// </summary>
    Profile,

    /// <summary>
    /// Displays the page with achievements list.
    /// </summary>
    Achievements,

    /// <summary>
    /// Displays the page with stats list.
    /// </summary>
    Stats,

    /// <summary>
    /// Displays the page with friends list.
    /// </summary>
    Friends,

    /// <summary>
    /// Displays the page with text chat.
    /// </summary>
    Chat,
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

    /// <summary>
    /// Gets the platform-specific identifier of this game. The same for all players playing this game. Can be used to check if the user is playing this game or a different one.
    /// </summary>
    /// <returns>Game identifier.</returns>
    API_FUNCTION() virtual Guid GetGameId() = 0;

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
    /// Gets the player avatar image.
    /// </summary>
    /// <remarks>Caller has to destroy the returned virtual texture.</remarks>
    /// <param name="user">The player to get avatar for.</param>
    /// <param name="avatar">The loaded avatar texture, null if cannot get it. Caller has to destroy the returned virtual texture once unused.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetUserAvatar(const OnlineUser& user, API_PARAM(Out) Texture*& avatar) = 0;

    /// <summary>
    /// Gets the list of friends of the user from the online platform.
    /// </summary>
    /// <param name="friends">The result local player friends user infos.</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetFriends(API_PARAM(Out) Array<OnlineUser, HeapAllocation>& friends, User* localUser = nullptr) = 0;

    /// <summary>
    /// Sets the rich-presence status text for the local user. Can be used to display custom status message in the friends list or other places.
    /// </summary>
    /// <param name="status">The status text. Empty to clear presence status.</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool SetPresence(const StringView& status, User* localUser = nullptr) = 0;

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
    /// <remarks>Only available in Development and Debug builds.</remarks>
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
    /// Gets the online leaderboard.
    /// </summary>
    /// <param name="name">The leaderboard name.</param>
    /// <param name="value">The result value.</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetLeaderboard(const StringView& name, API_PARAM(Out) OnlineLeaderboard& value, User* localUser = nullptr) { return true; }

    /// <summary>
    /// Gets or creates the online leaderboard. It will not create it if already exists.
    /// </summary>
    /// <param name="name">The leaderboard name.</param>
    /// <param name="sortMode">The leaderboard sorting mode.</param>
    /// <param name="valueFormat">The leaderboard values formatting.</param>
    /// <param name="value">The result value.</param>
    /// <param name="localUser">The local user (null if use default one).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetOrCreateLeaderboard(const StringView& name, OnlineLeaderboardSortModes sortMode, OnlineLeaderboardValueFormats valueFormat, API_PARAM(Out) OnlineLeaderboard& value, User* localUser = nullptr) { return true; }

    /// <summary>
    /// Gets the online leaderboard entries. Allows to specify the range for ranks to gather.
    /// </summary>
    /// <param name="leaderboard">The leaderboard.</param>
    /// <param name="entries">The list of result entries.</param>
    /// <param name="start">The zero-based index to start downloading entries from. For example, to display the top 10 on a leaderboard pass value of 0.</param>
    /// <param name="count">The amount of entries to read, starting from the first entry at <paramref name="start"/>.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetLeaderboardEntries(const OnlineLeaderboard& leaderboard, API_PARAM(Out) Array<OnlineLeaderboardEntry, HeapAllocation>& entries, int32 start = 0, int32 count = 10) { return true; }

    /// <summary>
    /// Gets the online leaderboard entries around the player. Allows to specify the range for ranks to gather around the user rank. The current user's entry is always included.
    /// </summary>
    /// <param name="leaderboard">The leaderboard.</param>
    /// <param name="entries">The list of result entries.</param>
    /// <param name="start">The zero-based index to start downloading entries relative to the user. For example, to display the 4 higher scores on a leaderboard pass value of -4. Value 0 will return current user as the first one.</param>
    /// <param name="count">The amount of entries to read, starting from the first entry at <paramref name="start"/>.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetLeaderboardEntriesAroundUser(const OnlineLeaderboard& leaderboard, API_PARAM(Out) Array<OnlineLeaderboardEntry, HeapAllocation>& entries, int32 start = -4, int32 count = 10) { return true; }

    /// <summary>
    /// Gets the online leaderboard entries for player friends.
    /// </summary>
    /// <param name="leaderboard">The leaderboard.</param>
    /// <param name="entries">The list of result entries.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetLeaderboardEntriesForFriends(const OnlineLeaderboard& leaderboard, API_PARAM(Out) Array<OnlineLeaderboardEntry, HeapAllocation>& entries) { return true; }

    /// <summary>
    /// Gets the online leaderboard entries for an arbitrary set of users.
    /// </summary>
    /// <param name="leaderboard">The leaderboard.</param>
    /// <param name="entries">The list of result entries.</param>
    /// <param name="users">The list of users to read their ranks on the leaderboard.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool GetLeaderboardEntriesForUsers(const OnlineLeaderboard& leaderboard, API_PARAM(Out) Array<OnlineLeaderboardEntry, HeapAllocation>& entries, const Array<OnlineUser, HeapAllocation>& users) { return true; }

    /// <summary>
    /// Sets the online leaderboard entry for the user.
    /// </summary>
    /// <param name="leaderboard">The leaderboard.</param>
    /// <param name="score">The score value to set.</param>
    /// <param name="keepBest">True if store value only if it's better than existing value (if any), otherwise will override any existing score for that user.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool SetLeaderboardEntry(const OnlineLeaderboard& leaderboard, int32 score, bool keepBest = false) { return true; }

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

public:
    /// <summary>
    /// Opens the game store overlay to a specific place. Avaliability depends on the platform and store.
    /// </summary>
    /// <param name="dialog">The dialog to display.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool OpenOverlay(OnlineOverlayDialog dialog) = 0;

    /// <summary>
    /// Opens the game store overlay to a specific place for a given website URL. Avaliability depends on the platform and store.
    /// </summary>
    /// <param name="url">The URL of the website to open. Full address with protocol type is required, e.g. https://www.flaxengine.com/.</param>
    /// <param name="dialog">The dialog to display.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool OpenOverlayUrl(const StringView& url, OnlineOverlayDialog dialog = OnlineOverlayDialog::Website) = 0;

    /// <summary>
    /// Opens the game store overlay to a specific place for a given user. Avaliability depends on the platform and store.
    /// </summary>
    /// <param name="user">The user to show Profile/Achievements/Stats pages of.</param>
    /// <param name="dialog">The dialog to display.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() virtual bool OpenOverlayUser(const OnlineUser& user, OnlineOverlayDialog dialog = OnlineOverlayDialog::Profile) = 0;
};
