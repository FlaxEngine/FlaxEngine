// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Engine/Base/ApplicationBase.h"

class Actor;
class SplashScreen;
class ProjectInfo;
class ManagedEditor;

static_assert(USE_EDITOR, "Don't include Editor in non-editor builds.");

/// <summary>
/// Main editor class
/// </summary>
class FLAXENGINE_API Editor : public ApplicationBase
{
public:

    /// <summary>
    /// The managed editor object.
    /// </summary>
    static ManagedEditor* Managed;

    /// <summary>
    /// Information about the loaded game project.
    /// </summary>
    static ProjectInfo* Project;

    /// <summary>
    /// True if the editor is running now in a play mode. Assigned by the managed editor instance.
    /// </summary>
    static bool IsPlayMode;

public:

    /// <summary>
    /// Closes the splash screen window (if visible).
    /// </summary>
    static void CloseSplashScreen();

public:

    /// <summary>
    /// The flag used to determine if a project was used with the older engine version last time it was opened. Some cached data should be regenerated to prevent version difference issues. The version number comparison is based on major and minor part of the version. Build number is ignored.
    /// </summary>
    static bool IsOldProjectOpened;

    /// <summary>
    /// The build number of the last editor instance that opened the current project. Value 0 means the undefined version.
    /// </summary>
    static int32 LastProjectOpenedEngineBuild;

    /// <summary>
    /// Checks if the project upgrade is required and may perform it. Will update IsOldProjectOpened flag.
    /// </summary>
    /// <returns>True if engine should exit, otherwise false.</returns>
    static bool CheckProjectUpgrade();

    /// <summary>
    /// Backups the whole project (all files) to the separate directory.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    static bool BackupProject();

public:

    // [ApplicationBase]
    static int32 LoadProduct();
    static Window* CreateMainWindow();
    static bool Init();
    static void BeforeRun();
    static void BeforeExit();
};
