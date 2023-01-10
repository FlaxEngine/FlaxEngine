// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "../Types.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"

/// <summary>
/// String container for names and typenames used by the managed runtime backend (8-bit chars).
/// </summary>
typedef StringAnsi MString;

enum class MVisibility
{
    Private,
    PrivateProtected,
    Internal,
    Protected,
    ProtectedInternal,
    Public,
};
