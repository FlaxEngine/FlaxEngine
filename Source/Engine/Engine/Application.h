// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "Editor/Editor.h"
typedef Editor Application;

#else

#include "Game.h"
typedef Game Application;

#endif
