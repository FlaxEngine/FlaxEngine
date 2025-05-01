// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

/// <summary>
/// Enumeration for key codes.
/// </summary>
API_ENUM() enum class KeyboardKeys
{
    /// <summary>
    /// The 'none' key
    /// </summary>
    None = 0,

    /// <summary>
    /// BACKSPACE key
    /// </summary>
    Backspace = 0x08,

    /// <summary>
    /// TAB key
    /// </summary>
    Tab = 0x09,

    /// <summary>
    /// CLEAR key
    /// </summary>
    Clear = 0x0C,

    /// <summary>
    /// ENTER key
    /// </summary>
    Return = 0x0D,

    /// <summary>
    /// Any SHIFT key, right or left
    /// </summary>
    Shift = 0x10,

    /// <summary>
    /// Any CTRL key, right or left
    /// </summary>
    Control = 0x11,

    /// <summary>
    /// Any ALT key, right or left
    /// </summary>
    Alt = 0x12,

    /// <summary>
    /// PAUSE key
    /// </summary>
    Pause = 0x13,

    /// <summary>
    /// CAPS LOCK key
    /// </summary>
    Capital = 0x14,

    /// <summary>
    /// IME Kana mode
    /// </summary>
    Kana = 0x15,

    /// <summary>
    /// IME Hangul mode
    /// </summary>
    Hangul = 0x15,

    /// <summary>
    /// IME Junja mode
    /// </summary>
    Junja = 0x17,

    /// <summary>
    /// IME final mode
    /// </summary>
    Final = 0x18,

    /// <summary>
    /// IME Hanja mode
    /// </summary>
    Hanja = 0x19,

    /// <summary>
    /// IME Kanji mode
    /// </summary>
    Kanji = 0x19,

    /// <summary>
    /// ESC key
    /// </summary>
    Escape = 0x1B,

    /// <summary>
    /// IME convert
    /// </summary>
    Convert = 0x1C,

    /// <summary>
    /// IME nonconvert
    /// </summary>
    Nonconvert = 0x1D,

    /// <summary>
    /// IME accept
    /// </summary>
    Accept = 0x1E,

    /// <summary>
    /// IME mode change request
    /// </summary>
    Modechange = 0x1F,

    /// <summary>
    /// SPACEBAR key
    /// </summary>
    Spacebar = 0x20,

    /// <summary>
    /// PAGE UP key
    /// </summary>
    PageUp = 0x21,

    /// <summary>
    /// PAGE DOWN key
    /// </summary>
    PageDown = 0x22,

    /// <summary>
    /// END key
    /// </summary>
    End = 0x23,

    /// <summary>
    /// HOME key
    /// </summary>
    Home = 0x24,

    /// <summary>
    /// LEFT ARROW key
    /// </summary>
    ArrowLeft = 0x25,

    /// <summary>
    /// UP ARROW key
    /// </summary>
    ArrowUp = 0x26,

    /// <summary>
    /// RIGHT ARROW key
    /// </summary>
    ArrowRight = 0x27,

    /// <summary>
    /// DOWN ARROW key
    /// </summary>
    ArrowDown = 0x28,

    /// <summary>
    /// SELECT key
    /// </summary>
    Select = 0x29,

    /// <summary>
    /// PRINT key
    /// </summary>
    Print = 0x2A,

    /// <summary>
    /// EXECUTE key
    /// </summary>
    Execute = 0x2B,

    /// <summary>
    /// PRINT SCREEN key
    /// </summary>
    PrintScreen = 0x2C,

    /// <summary>
    /// INSERT key
    /// </summary>
    Insert = 0x2D,

    /// <summary>
    /// DELETE key
    /// </summary>
    Delete = 0x2E,

    /// <summary>
    /// HELP key
    /// </summary>
    Help = 0x2F,

    /// <summary>
    /// The '0' key on the top of the alphanumeric keyboard.
    /// </summary>
    Alpha0 = 0x30,

    /// <summary>
    /// The '1' key on the top of the alphanumeric keyboard.
    /// </summary>
    Alpha1 = 0x31,

    /// <summary>
    /// The '2' key on the top of the alphanumeric keyboard.
    /// </summary>
    Alpha2 = 0x32,

    /// <summary>
    /// The '3' key on the top of the alphanumeric keyboard.
    /// </summary>
    Alpha3 = 0x33,

    /// <summary>
    /// The '4' key on the top of the alphanumeric keyboard.
    /// </summary>
    Alpha4 = 0x34,

    /// <summary>
    /// The '5' key on the top of the alphanumeric keyboard.
    /// </summary>
    Alpha5 = 0x35,

    /// <summary>
    /// The '6' key on the top of the alphanumeric keyboard.
    /// </summary>
    Alpha6 = 0x36,

    /// <summary>
    /// The '7' key on the top of the alphanumeric keyboard.
    /// </summary>
    Alpha7 = 0x37,

    /// <summary>
    /// The '8' key on the top of the alphanumeric keyboard.
    /// </summary>
    Alpha8 = 0x38,

    /// <summary>
    /// The '9' key on the top of the alphanumeric keyboard.
    /// </summary>
    Alpha9 = 0x39,

    /// <summary>
    /// A key
    /// </summary>
    A = 0x41,

    /// <summary>
    /// B key
    /// </summary>
    B = 0x42,

    /// <summary>
    /// C key
    /// </summary>
    C = 0x43,

    /// <summary>
    /// D key
    /// </summary>
    D = 0x44,

    /// <summary>
    /// E key
    /// </summary>
    E = 0x45,

    /// <summary>
    /// F key
    /// </summary>
    F = 0x46,

    /// <summary>
    /// G key
    /// </summary>
    G = 0x47,

    /// <summary>
    /// H key
    /// </summary>
    H = 0x48,

    /// <summary>
    /// I key
    /// </summary>
    I = 0x49,

    /// <summary>
    /// J key
    /// </summary>
    J = 0x4A,

    /// <summary>
    /// K key
    /// </summary>
    K = 0x4B,

    /// <summary>
    /// L key
    /// </summary>
    L = 0x4C,

    /// <summary>
    /// M key
    /// </summary>
    M = 0x4D,

    /// <summary>
    /// N key
    /// </summary>
    N = 0x4E,

    /// <summary>
    /// O key
    /// </summary>
    O = 0x4F,

    /// <summary>
    /// P key
    /// </summary>
    P = 0x50,

    /// <summary>
    /// Q key
    /// </summary>
    Q = 0x51,

    /// <summary>
    /// R key
    /// </summary>
    R = 0x52,

    /// <summary>
    /// S key
    /// </summary>
    S = 0x53,

    /// <summary>
    /// T key
    /// </summary>
    T = 0x54,

    /// <summary>
    /// U key
    /// </summary>
    U = 0x55,

    /// <summary>
    /// V key
    /// </summary>
    V = 0x56,

    /// <summary>
    /// W key
    /// </summary>
    W = 0x57,

    /// <summary>
    /// X key
    /// </summary>
    X = 0x58,

    /// <summary>
    /// Y key
    /// </summary>
    Y = 0x59,

    /// <summary>
    /// Z key
    /// </summary>
    Z = 0x5A,

    /// <summary>
    /// Left Windows key (Natural keyboard)
    /// </summary>
    LeftWindows = 0x5B,

    /// <summary>
    /// Right Windows key (Natural keyboard)
    /// </summary>
    RightWindows = 0x5C,

    /// <summary>
    /// Applications key (Natural keyboard)
    /// </summary>
    Applications = 0x5D,

    /// <summary>
    /// Computer Sleep key
    /// </summary>
    Sleep = 0x5F,

    /// <summary>
    /// Numeric keypad 0 key
    /// </summary>
    Numpad0 = 0x60,

    /// <summary>
    /// Numeric keypad 1 key
    /// </summary>
    Numpad1 = 0x61,

    /// <summary>
    /// Numeric keypad 2 key
    /// </summary>
    Numpad2 = 0x62,

    /// <summary>
    /// Numeric keypad 3 key
    /// </summary>
    Numpad3 = 0x63,

    /// <summary>
    /// Numeric keypad 4 key
    /// </summary>
    Numpad4 = 0x64,

    /// <summary>
    /// Numeric keypad 5 key
    /// </summary>
    Numpad5 = 0x65,

    /// <summary>
    /// Numeric keypad 6 key
    /// </summary>
    Numpad6 = 0x66,

    /// <summary>
    /// Numeric keypad 7 key
    /// </summary>
    Numpad7 = 0x67,

    /// <summary>
    /// Numeric keypad 8 key
    /// </summary>
    Numpad8 = 0x68,

    /// <summary>
    /// Numeric keypad 9 key
    /// </summary>
    Numpad9 = 0x69,

    /// <summary>
    /// Numeric keypad Multiply key '*'
    /// </summary>
    NumpadMultiply = 0x6A,

    /// <summary>
    /// Numeric keypad Add key '+'
    /// </summary>
    NumpadAdd = 0x6B,

    /// <summary>
    /// Numeric Separator key
    /// </summary>
    NumpadSeparator = 0x6C,

    /// <summary>
    /// Numeric keypad Subtract key '-'
    /// </summary>
    NumpadSubtract = 0x6D,

    /// <summary>
    /// Numeric keypad Decimal key ','
    /// </summary>
    NumpadDecimal = 0x6E,

    /// <summary>
    /// Numeric keypad Divide key '/'
    /// </summary>
    NumpadDivide = 0x6F,

    /// <summary>
    /// F1 function key
    /// </summary>
    F1 = 0x70,

    /// <summary>
    /// F2 function key
    /// </summary>
    F2 = 0x71,

    /// <summary>
    /// F3 function key
    /// </summary>
    F3 = 0x72,

    /// <summary>
    /// F4 function key
    /// </summary>
    F4 = 0x73,

    /// <summary>
    /// F5 function key
    /// </summary>
    F5 = 0x74,

    /// <summary>
    /// F6 function key
    /// </summary>
    F6 = 0x75,

    /// <summary>
    /// F7 function key
    /// </summary>
    F7 = 0x76,

    /// <summary>
    /// F8 function key
    /// </summary>
    F8 = 0x77,

    /// <summary>
    /// F9 function key
    /// </summary>
    F9 = 0x78,

    /// <summary>
    /// F10 function key
    /// </summary>
    F10 = 0x79,

    /// <summary>
    /// F11 function key
    /// </summary>
    F11 = 0x7A,

    /// <summary>
    /// F12 function key
    /// </summary>
    F12 = 0x7B,

    /// <summary>
    /// F13 function key
    /// </summary>
    F13 = 0x7C,

    /// <summary>
    /// F14 function key
    /// </summary>
    F14 = 0x7D,

    /// <summary>
    /// F15 function key
    /// </summary>
    F15 = 0x7E,

    /// <summary>
    /// F16 function key
    /// </summary>
    F16 = 0x7F,

    /// <summary>
    /// F17 function key
    /// </summary>
    F17 = 0x80,

    /// <summary>
    /// F18 function key
    /// </summary>
    F18 = 0x81,

    /// <summary>
    /// F19 function key
    /// </summary>
    F19 = 0x82,

    /// <summary>
    /// F20 function key
    /// </summary>
    F20 = 0x83,

    /// <summary>
    /// F21 function key
    /// </summary>
    F21 = 0x84,

    /// <summary>
    /// F22 function key
    /// </summary>
    F22 = 0x85,

    /// <summary>
    /// F23 function key
    /// </summary>
    F23 = 0x86,

    /// <summary>
    /// F24 function key
    /// </summary>
    F24 = 0x87,

    /// <summary>
    /// Numeric keypad NUM LOCK key
    /// </summary>
    Numlock = 0x90,

    /// <summary>
    /// SCROLL LOCK key
    /// </summary>
    Scroll = 0x91,

    /// <summary>
    /// Left MENU key
    /// </summary>
    LeftMenu = 0xA4,

    /// <summary>
    /// Right MENU key
    /// </summary>
    RightMenu = 0xA5,

    /// <summary>
    /// Browser Back key
    /// </summary>
    BrowserBack = 0xA6,

    /// <summary>
    /// Browser Forward key
    /// </summary>
    BrowserForward = 0xA7,

    /// <summary>
    /// Browser Refresh key
    /// </summary>
    BrowserRefresh = 0xA8,

    /// <summary>
    /// Browser Stop key
    /// </summary>
    BrowserStop = 0xA9,

    /// <summary>
    /// Browser Search key
    /// </summary>
    BrowserSearch = 0xAA,

    /// <summary>
    /// Browser Favorites key
    /// </summary>
    BrowserFavorites = 0xAB,

    /// <summary>
    /// Browser Start and Home key
    /// </summary>
    BrowserHome = 0xAC,

    /// <summary>
    /// Volume Mute key
    /// </summary>
    VolumeMute = 0xAD,

    /// <summary>
    /// Volume Down key
    /// </summary>
    VolumeDown = 0xAE,

    /// <summary>
    /// Volume Up key
    /// </summary>
    VolumeUp = 0xAF,

    /// <summary>
    /// Next Track key
    /// </summary>
    MediaNextTrack = 0xB0,

    /// <summary>
    /// Previous Track key
    /// </summary>
    MediaPrevTrack = 0xB1,

    /// <summary>
    /// Stop Media key
    /// </summary>
    MediaStop = 0xB2,

    /// <summary>
    /// Play/Pause Media key
    /// </summary>
    MediaPlayPause = 0xB3,

    /// <summary>
    /// Start Mail key
    /// </summary>
    LaunchMail = 0xB4,

    /// <summary>
    /// Select Media key
    /// </summary>
    LaunchMediaSelect = 0xB5,

    /// <summary>
    /// Start Application 1 key
    /// </summary>
    LaunchApp1 = 0xB6,

    /// <summary>
    /// Start Application 2 key
    /// </summary>
    LaunchApp2 = 0xB7,

    /// <summary>
    /// Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard the ';:' key
    /// </summary>
    Colon = 0xBA,

    /// <summary>
    /// For any country/region the '+' key
    /// </summary>
    Plus = 0xBB,

    /// <summary>
    /// For any country/region the ',' key
    /// </summary>
    Comma = 0xBC,

    /// <summary>
    /// For any country/region the '-' key
    /// </summary>
    Minus = 0xBD,

    /// <summary>
    /// For any country/region the '.' key
    /// </summary>
    Period = 0xBE,

    /// <summary>
    /// Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard the '/?' key
    /// </summary>
    Slash = 0xBF,

    /// <summary>
    /// Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard the '`~' key
    /// </summary>
    BackQuote = 0xC0,

    /// <summary>
    /// Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard the '[{' key
    /// </summary>
    LeftBracket = 0xDB,

    /// <summary>
    /// Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard the '\|' key
    /// </summary>
    Backslash = 0xDC,

    /// <summary>
    /// Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard the ']}' key
    /// </summary>
    RightBracket = 0xDD,

    /// <summary>
    /// Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard the
    /// 'single-quote/double-quote' key
    /// </summary>
    Quote = 0xDE,

    /// <summary>
    /// Used for miscellaneous characters; it can vary by keyboard.
    /// </summary>
    Oem8 = 0xDF,

    /// <summary>
    /// Either the angle bracket key or the backslash key on the RT 102-key keyboard
    /// </summary>
    Oem102 = 0xE2,

    /// <summary>
    /// IME PROCESS key
    /// </summary>
    Processkey = 0xE5,

    /// <summary>
    /// Used to pass Unicode characters as if they were keystrokes. The PACKET key is the low word of a 32-bit Virtual Key
    /// value used for non-keyboard input methods.
    /// </summary>
    Packet = 0xE7,

    /// <summary>
    /// Attn key
    /// </summary>
    Attn = 0xF6,

    /// <summary>
    /// CrSel key
    /// </summary>
    Crsel = 0xF7,

    /// <summary>
    /// ExSel key
    /// </summary>
    Exsel = 0xF8,

    /// <summary>
    /// Erase EOF key
    /// </summary>
    Ereof = 0xF9,

    /// <summary>
    /// Play key
    /// </summary>
    Play = 0xFA,

    /// <summary>
    /// Zoom key
    /// </summary>
    Zoom = 0xFB,

    /// <summary>
    /// PA1 key
    /// </summary>
    Pa1 = 0xFD,

    /// <summary>
    /// Clear key
    /// </summary>
    OemClear = 0xFE,

    MAX
};
