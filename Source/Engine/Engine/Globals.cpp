// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
String Globals::MonoPath;
bool Globals::FatalErrorOccurred;
bool Globals::IsRequestingExit;
int32 Globals::ExitCode;
uint64 Globals::MainThreadID;
String Globals::EngineVersion(TEXT(FLAXENGINE_VERSION_TEXT));
int32 Globals::EngineBuildNumber = FLAXENGINE_VERSION_BUILD;
String Globals::ProductName;
String Globals::CompanyName;
int32 Globals::ContentKey;
