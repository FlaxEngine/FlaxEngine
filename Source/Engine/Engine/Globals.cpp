// Copyright (c) Wojciech Figat. All rights reserved.

#include "Globals.h"
#include "Engine/Core/Types/String.h"
#include "FlaxEngine.Gen.h"

String Globals::StartupFolder;
String Globals::TemporaryFolder;
String Globals::ProjectFolder;
String Globals::ProductLocalFolder;
String Globals::BinariesFolder;
#if USE_EDITOR
String Globals::ProjectCacheFolder;
String Globals::EngineContentFolder;
String Globals::ProjectSourceFolder;
#endif
String Globals::ProjectContentFolder;
#if USE_MONO
String Globals::MonoPath;
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS;
bool Globals::FatalErrorOccurred;
bool Globals::IsRequestingExit;
int32 Globals::ExitCode;
PRAGMA_ENABLE_DEPRECATION_WARNINGS;
uint64 Globals::MainThreadID;
String Globals::EngineVersion(TEXT(FLAXENGINE_VERSION_TEXT));
int32 Globals::EngineBuildNumber = FLAXENGINE_VERSION_BUILD;
String Globals::ProductName;
String Globals::CompanyName;
int32 Globals::ContentKey;
