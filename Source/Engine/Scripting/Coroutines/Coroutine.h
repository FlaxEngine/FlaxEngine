// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "CoroutineSuspensionPointIndex.h"
#include "CoroutineSuspensionPointsFlags.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Scripting/ScriptingObject.h"


API_CLASS(Sealed) class FLAXENGINE_API Coroutine final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(Coroutine, ScriptingObject);

    static void Example();
};
