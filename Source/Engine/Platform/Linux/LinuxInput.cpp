// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX

#include "LinuxInput.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/Keyboard.h"

/// <summary>
/// Implementation of the keyboard device for Linux platform.
/// </summary>
/// <seealso cref="Keyboard" />
class LinuxKeyboard : public Keyboard
{
public:

	/// <summary>
	/// Initializes a new instance of the <see cref="LinuxKeyboard"/> class.
	/// </summary>
	explicit LinuxKeyboard()
		: Keyboard()
	{
	}
};

/// <summary>
/// Implementation of the mouse device for Linux platform.
/// </summary>
/// <seealso cref="Mouse" />
class LinuxMouse : public Mouse
{
public:

	/// <summary>
	/// Initializes a new instance of the <see cref="LinuxMouse"/> class.
	/// </summary>
	explicit LinuxMouse()
		: Mouse()
	{
	}

public:

    // [Mouse]
    void SetMousePosition(const Vector2& newPosition) final override
    {
        LinuxPlatform::SetMousePosition(newPosition);

        OnMouseMoved(newPosition);
    }
};

namespace Impl
{
	LinuxKeyboard Keyboard;
	LinuxMouse Mouse;
}

void LinuxInput::Init()
{
    Input::Mouse = &Impl::Mouse;
    Input::Keyboard = &Impl::Keyboard;
}

#endif
