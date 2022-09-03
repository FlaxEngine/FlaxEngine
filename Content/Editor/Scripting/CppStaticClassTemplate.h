%copyright%#pragma once

#include "Engine/Scripting/Script.h"
#include "Engine/Core/Math/Vector4.h"

/// <summary>
/// %class% Function Library
/// </summary>
API_CLASS(Static) class %module%%class% 
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(%class%);
public:

    /// <summary>
    /// Logs the function parameter natively.
    /// </summary>
    /// <param name="data">Data to pass to native code</param>
    API_FUNCTION() static void RunNativeAction(Vector4 data);
};
