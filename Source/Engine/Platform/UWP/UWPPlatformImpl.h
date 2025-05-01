// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UWP

/// <summary>
/// Represents the current state of the UWP gamepad device.
/// </summary>
struct UWPGamepadState
{
	int Buttons;
	float LeftThumbstickX;
	float LeftThumbstickY;
	float RightThumbstickX;
	float RightThumbstickY;
	float LeftTrigger;
	float RightTrigger;
};

struct UWPGamepadStateVibration
{
	float LeftLarge;
	float LeftSmall;
	float RightLarge;
	float RightSmall;
};

/// <summary>
/// Base interface for Universal Windows Platform (UWP) window implementation. Separated from the FlaxEngine main lib (uses C++ /CX).
/// </summary>
class UWPWindowImpl
{
public:

	enum class CursorType
	{
		Default = 0,
		Cross,
		Hand,
		Help,
		IBeam,
		No,
		Wait,
		SizeAll,
		SizeNESW,
		SizeNS,
		SizeNWSE,
		SizeWE,
		Hidden
	};

public:

	UWPWindowImpl()
	{
		UserData = nullptr;
	}

	/// <summary>
	/// Finalizes an instance of the <see cref="UWPWindowImpl"/> class.
	/// </summary>
	virtual ~UWPWindowImpl()
	{
	}

public:

	/// <summary>
	/// Custom user data. Passed in the events. Used to keep ref to the UWPWindow.
	/// </summary>
	void* UserData;

public:

	struct PointerData
	{
		unsigned int PointerId;
		float PositionX;
		float PositionY;
		int MouseWheelDelta;

		int IsLeftButtonPressed : 1;
		int IsMiddleButtonPressed : 1;
		int IsRightButtonPressed : 1;
		int IsXButton1Pressed : 1;
		int IsXButton2Pressed : 1;
		int IsMouse : 1;
		int IsPen : 1;
		int IsTouch : 1;
	};

	typedef void(*WinEvent)(void* userData);
	typedef void(*WinVisibleEvent)(bool visible, void* userData);
	typedef void(*WinResizeEvent)(float width, float height, void* userData);
	typedef void(*WinDpiEvent)(float dpi, void* userData);
	typedef void(*WinFocusEvent)(bool focused, void* userData);
	typedef void(*WinKeyEvent)(int key, void* userData);
	typedef void(*WinMouseMovedEvent)(float x, float y, void* userData);
	typedef void(*WinPointerEvent)(PointerData* pointer, void* userData);

	WinResizeEvent SizeChanged = nullptr;
	WinVisibleEvent VisibilityChanged = nullptr;
	WinDpiEvent DpiChanged = nullptr;
	WinEvent Closed = nullptr;
	WinFocusEvent FocusChanged = nullptr;
	WinKeyEvent KeyDown = nullptr;
	WinKeyEvent KeyUp = nullptr;
	WinKeyEvent CharacterReceived = nullptr;
	WinMouseMovedEvent MouseMoved = nullptr;
	WinPointerEvent PointerPressed = nullptr;
	WinPointerEvent PointerMoved = nullptr;
	WinPointerEvent PointerWheelChanged = nullptr;
	WinPointerEvent PointerReleased = nullptr;
	WinPointerEvent PointerExited = nullptr;

public:

	virtual void* GetHandle() const = 0;
	virtual void SetCursor(CursorType type) = 0;
	virtual void SetMousePosition(float x, float y) = 0;
	virtual void GetMousePosition(float* x, float* y) = 0;
	virtual void GetBounds(float* x, float* y, float* width, float* height) = 0;
	virtual void GetDpi(int* dpi) = 0;
	virtual void GetTitle(wchar_t* buffer, int bufferLength) = 0;
	virtual void SetTitle(const wchar_t* title) = 0;
	virtual int GetGamepadsCount() = 0;
	virtual void SetGamepadVibration(const int index, UWPGamepadStateVibration& vibration) = 0;
	virtual void GetGamepadState(const int index, UWPGamepadState& state) = 0;
	virtual void Activate() = 0;
	virtual void Close() = 0;
};

/// <summary>
/// Base interface for Universal Windows Platform (UWP) implementation. Separated from the FlaxEngine main lib (uses C++ /CX).
/// </summary>
class UWPPlatformImpl
{
public:

	enum class DialogResult
	{
		Abort = 0,
		Cancel = 1,
		Ignore = 2,
		No = 3,
		None = 4,
		OK = 5,
		Retry = 6,
		Yes = 7
	};

	enum class MessageBoxIcon
	{
		Asterisk = 0,
		Error = 1,
		Exclamation = 2,
		Hand = 3,
		Information = 4,
		None = 5,
		Question = 6,
		Stop = 7,
		Warning = 8
	};

	enum class MessageBoxButtons
	{
		AbortRetryIgnore = 0,
		OK = 1,
		OKCancel = 2,
		RetryCancel = 3,
		YesNo = 4,
		YesNoCancel = 5
	};

	enum class SpecialFolder
	{
		Desktop = 0,
		Documents = 1,
		Pictures = 2,
		AppData = 3,
		LocalAppData = 4,
		ProgramData = 5,
		Temporary = 6,
	};

public:

	virtual UWPWindowImpl* GetMainWindowImpl() = 0;
	virtual void Tick() = 0;
	virtual int GetDpi() = 0;
	virtual int GetSpecialFolderPath(const SpecialFolder type, wchar_t* buffer, int bufferLength) = 0;
	virtual void GetDisplaySize(float* width, float* height) = 0;
	virtual DialogResult ShowMessageDialog(UWPWindowImpl* window, const wchar_t* text, const wchar_t* caption, MessageBoxButtons buttons, MessageBoxIcon icon) = 0;
};

extern UWPPlatformImpl* CUWPPlatform;

extern void RunUWP();

#endif
