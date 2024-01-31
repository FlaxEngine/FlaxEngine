//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"
/// <summary>
/// the UI response to the Event OnPointer/OnAction
/// </summary>
API_ENUM(Namespace = "FlaxEngine.Experimental.UI")
enum class UIEventResponse
{
	/// <summary>
	/// The none
	/// </summary>
	None = 0,
	/// <summary>
	/// The focuses on element and consumes the event
	/// </summary>
	Focus = 1,
	/// <summary>
	/// The captures the events and consumes the event
	/// </summary>
	Capture = 2,
};
