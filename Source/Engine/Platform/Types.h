// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS

class Win32CriticalSection;
typedef Win32CriticalSection CriticalSection;
class Win32ConditionVariable;
typedef Win32ConditionVariable ConditionVariable;
class WindowsFileSystem;
typedef WindowsFileSystem FileSystem;
class WindowsFileSystemWatcher;
typedef WindowsFileSystemWatcher FileSystemWatcher;
class Win32File;
typedef Win32File File;
class Win32Thread;
typedef Win32Thread Thread;
class WindowsClipboard;
typedef WindowsClipboard Clipboard;
#if !PLATFORM_SDL
class WindowsPlatform;
typedef WindowsPlatform Platform;
class WindowsWindow;
typedef WindowsWindow Window;
#endif
class Win32Network;
typedef Win32Network Network;
class UserBase;
typedef UserBase User;
class WindowsScreenUtilities;
typedef WindowsScreenUtilities ScreenUtilities;

#elif PLATFORM_UWP

class Win32CriticalSection;
typedef Win32CriticalSection CriticalSection;
class Win32ConditionVariable;
typedef Win32ConditionVariable ConditionVariable;
class UWPFileSystem;
typedef UWPFileSystem FileSystem;
class FileSystemWatcherBase;
typedef FileSystemWatcherBase FileSystemWatcher;
class Win32File;
typedef Win32File File;
class Win32Thread;
typedef Win32Thread Thread;
class ClipboardBase;
typedef ClipboardBase Clipboard;
class UWPPlatform;
typedef UWPPlatform Platform;
class UWPWindow;
typedef UWPWindow Window;
class Win32Network;
typedef Win32Network Network;
class UserBase;
typedef UserBase User;

#elif PLATFORM_LINUX

class UnixCriticalSection;
typedef UnixCriticalSection CriticalSection;
class UnixConditionVariable;
typedef UnixConditionVariable ConditionVariable;
class LinuxFileSystem;
typedef LinuxFileSystem FileSystem;
class LinuxFileSystemWatcher;
typedef LinuxFileSystemWatcher FileSystemWatcher;
class UnixFile;
typedef UnixFile File;
class LinuxThread;
typedef LinuxThread Thread;
#if !PLATFORM_SDL
class LinuxClipboard;
typedef LinuxClipboard Clipboard;
class LinuxPlatform;
typedef LinuxPlatform Platform;
class LinuxWindow;
typedef LinuxWindow Window;
#endif
class UnixNetwork;
typedef UnixNetwork Network;
class UserBase;
typedef UserBase User;
class LinuxScreenUtilities;
typedef LinuxScreenUtilities ScreenUtilities;

#elif PLATFORM_PS4

class UnixCriticalSection;
typedef UnixCriticalSection CriticalSection;
class UnixConditionVariable;
typedef UnixConditionVariable ConditionVariable;
class PS4FileSystem;
typedef PS4FileSystem FileSystem;
class FileSystemWatcherBase;
typedef FileSystemWatcherBase FileSystemWatcher;
class UnixFile;
typedef UnixFile File;
class PS4Thread;
typedef PS4Thread Thread;
class ClipboardBase;
typedef ClipboardBase Clipboard;
class PS4Platform;
typedef PS4Platform Platform;
class PS4Window;
typedef PS4Window Window;
class PS4Network;
typedef PS4Network Network;
class PS4User;
typedef PS4User User;

#elif PLATFORM_PS5

class UnixCriticalSection;
typedef UnixCriticalSection CriticalSection;
class UnixConditionVariable;
typedef UnixConditionVariable ConditionVariable;
class PS5FileSystem;
typedef PS5FileSystem FileSystem;
class FileSystemWatcherBase;
typedef FileSystemWatcherBase FileSystemWatcher;
class UnixFile;
typedef UnixFile File;
class ClipboardBase;
typedef ClipboardBase Clipboard;
class PS5Thread;
typedef PS5Thread Thread;
class PS5Platform;
typedef PS5Platform Platform;
class PS5Window;
typedef PS5Window Window;
class PS5Network;
typedef PS5Network Network;
class PS5User;
typedef PS5User User;

#elif PLATFORM_XBOX_ONE

class Win32CriticalSection;
typedef Win32CriticalSection CriticalSection;
class Win32ConditionVariable;
typedef Win32ConditionVariable ConditionVariable;
class XboxOneFileSystem;
typedef XboxOneFileSystem FileSystem;
class FileSystemWatcherBase;
typedef FileSystemWatcherBase FileSystemWatcher;
class Win32File;
typedef Win32File File;
class Win32Thread;
typedef Win32Thread Thread;
class ClipboardBase;
typedef ClipboardBase Clipboard;
class XboxOnePlatform;
typedef XboxOnePlatform Platform;
class GDKWindow;
typedef GDKWindow Window;
class Win32Network;
typedef Win32Network Network;
class GDKUser;
typedef GDKUser User;

#elif PLATFORM_XBOX_SCARLETT

class Win32CriticalSection;
typedef Win32CriticalSection CriticalSection;
class Win32ConditionVariable;
typedef Win32ConditionVariable ConditionVariable;
class XboxScarlettFileSystem;
typedef XboxScarlettFileSystem FileSystem;
class FileSystemWatcherBase;
typedef FileSystemWatcherBase FileSystemWatcher;
class Win32File;
typedef Win32File File;
class Win32Thread;
typedef Win32Thread Thread;
class ClipboardBase;
typedef ClipboardBase Clipboard;
class XboxScarlettPlatform;
typedef XboxScarlettPlatform Platform;
class GDKWindow;
typedef GDKWindow Window;
class Win32Network;
typedef Win32Network Network;
class GDKUser;
typedef GDKUser User;

#elif PLATFORM_ANDROID

class UnixCriticalSection;
typedef UnixCriticalSection CriticalSection;
class UnixConditionVariable;
typedef UnixConditionVariable ConditionVariable;
class AndroidFileSystem;
typedef AndroidFileSystem FileSystem;
class FileSystemWatcherBase;
typedef FileSystemWatcherBase FileSystemWatcher;
class AndroidFile;
typedef AndroidFile File;
class AndroidThread;
typedef AndroidThread Thread;
class ClipboardBase;
typedef ClipboardBase Clipboard;
class AndroidPlatform;
typedef AndroidPlatform Platform;
class AndroidWindow;
typedef AndroidWindow Window;
class UnixNetwork;
typedef UnixNetwork Network;
class UserBase;
typedef UserBase User;

#elif PLATFORM_SWITCH

class SwitchCriticalSection;
typedef SwitchCriticalSection CriticalSection;
class SwitchConditionVariable;
typedef SwitchConditionVariable ConditionVariable;
class SwitchFileSystem;
typedef SwitchFileSystem FileSystem;
class FileSystemWatcherBase;
typedef FileSystemWatcherBase FileSystemWatcher;
class SwitchFile;
typedef SwitchFile File;
class SwitchThread;
typedef SwitchThread Thread;
class ClipboardBase;
typedef ClipboardBase Clipboard;
class SwitchPlatform;
typedef SwitchPlatform Platform;
class SwitchWindow;
typedef SwitchWindow Window;
class SwitchNetwork;
typedef SwitchNetwork Network;
class SwitchUser;
typedef SwitchUser User;

#elif PLATFORM_MAC

class UnixCriticalSection;
typedef UnixCriticalSection CriticalSection;
class UnixConditionVariable;
typedef UnixConditionVariable ConditionVariable;
class MacFileSystem;
typedef MacFileSystem FileSystem;
class MacFileSystemWatcher;
typedef MacFileSystemWatcher FileSystemWatcher;
class UnixFile;
typedef UnixFile File;
class AppleThread;
typedef AppleThread Thread;
class MacClipboard;
typedef MacClipboard Clipboard;
class MacPlatform;
typedef MacPlatform Platform;
class MacWindow;
typedef MacWindow Window;
class UnixNetwork;
typedef UnixNetwork Network;
class UserBase;
typedef UserBase User;
class MacScreenUtilities;
typedef MacScreenUtilities ScreenUtilities;

#elif PLATFORM_IOS

class UnixCriticalSection;
typedef UnixCriticalSection CriticalSection;
class UnixConditionVariable;
typedef UnixConditionVariable ConditionVariable;
class iOSFileSystem;
typedef iOSFileSystem FileSystem;
class FileSystemWatcherBase;
typedef FileSystemWatcherBase FileSystemWatcher;
class iOSFile;
typedef iOSFile File;
class AppleThread;
typedef AppleThread Thread;
class ClipboardBase;
typedef ClipboardBase Clipboard;
class iOSPlatform;
typedef iOSPlatform Platform;
class iOSWindow;
typedef iOSWindow Window;
class UnixNetwork;
typedef UnixNetwork Network;
class UserBase;
typedef UserBase User;

#else

#error Missing Types implementation!

#endif

#if PLATFORM_SDL
#if PLATFORM_LINUX
class SDLClipboard;
typedef SDLClipboard Clipboard;
#endif
class SDLPlatform;
typedef SDLPlatform Platform;
class SDLWindow;
typedef SDLWindow Window;
#endif
