// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS

class WindowsClipboard;
typedef WindowsClipboard Clipboard;
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
class WindowsPlatform;
typedef WindowsPlatform Platform;
class Win32Thread;
typedef Win32Thread Thread;
class WindowsWindow;
typedef WindowsWindow Window;
class Win32Network;
typedef Win32Network Network;

#elif PLATFORM_UWP

class ClipboardBase;
typedef ClipboardBase Clipboard;
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
class UWPPlatform;
typedef UWPPlatform Platform;
class Win32Thread;
typedef Win32Thread Thread;
class UWPWindow;
typedef UWPWindow Window;
class NetworkBase;
typedef NetworkBase Network;

#elif PLATFORM_LINUX

class LinuxClipboard;
typedef LinuxClipboard Clipboard;
class UnixCriticalSection;
typedef UnixCriticalSection CriticalSection;
class UnixConditionVariable;
typedef UnixConditionVariable ConditionVariable;
class LinuxFileSystem;
typedef LinuxFileSystem FileSystem;
class FileSystemWatcherBase;
typedef FileSystemWatcherBase FileSystemWatcher;
class UnixFile;
typedef UnixFile File;
class LinuxPlatform;
typedef LinuxPlatform Platform;
class LinuxThread;
typedef LinuxThread Thread;
class LinuxWindow;
typedef LinuxWindow Window;
class NetworkBase;
typedef NetworkBase Network;

#elif PLATFORM_PS4

class ClipboardBase;
typedef ClipboardBase Clipboard;
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
class PS4Platform;
typedef PS4Platform Platform;
class PS4Thread;
typedef PS4Thread Thread;
class PS4Window;
typedef PS4Window Window;
class NetworkBase;
typedef NetworkBase Network;

#elif PLATFORM_XBOX_SCARLETT

class ClipboardBase;
typedef ClipboardBase Clipboard;
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
class XboxScarlettPlatform;
typedef XboxScarlettPlatform Platform;
class Win32Thread;
typedef Win32Thread Thread;
class XboxScarlettWindow;
typedef XboxScarlettWindow Window;
class NetworkBase;
typedef NetworkBase Network;

#elif PLATFORM_ANDROID

class ClipboardBase;
typedef ClipboardBase Clipboard;
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
class AndroidPlatform;
typedef AndroidPlatform Platform;
class AndroidThread;
typedef AndroidThread Thread;
class AndroidWindow;
typedef AndroidWindow Window;
class NetworkBase;
typedef NetworkBase Network;

#else

#error Missing Types implementation!

#endif
