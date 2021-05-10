%copyright%
#pragma once

#include <Engine/Scripting/Script.h>
#include <Engine/Core/Math/Vector4.h>
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
    /// <param name="data">Vector4 parameter</param>
    /// <returns>void</returns>
    API_FUNCTION() void RunNativeAction(Vector4 data);
};
