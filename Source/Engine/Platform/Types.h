// Copyright (c) Wojciech Figat. All rights reserved.

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
class UserBase;
typedef UserBase User;

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
class Win32Network;
typedef Win32Network Network;
class UserBase;
typedef UserBase User;

#elif PLATFORM_LINUX

class LinuxClipboard;
typedef LinuxClipboard Clipboard;
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
class LinuxPlatform;
typedef LinuxPlatform Platform;
class LinuxThread;
typedef LinuxThread Thread;
class LinuxWindow;
typedef LinuxWindow Window;
class UnixNetwork;
typedef UnixNetwork Network;
class UserBase;
typedef UserBase User;

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
class PS4Network;
typedef PS4Network Network;
class PS4User;
typedef PS4User User;

#elif PLATFORM_PS5

class ClipboardBase;
typedef ClipboardBase Clipboard;
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
class PS5Platform;
typedef PS5Platform Platform;
class PS5Thread;
typedef PS5Thread Thread;
class PS5Window;
typedef PS5Window Window;
class PS5Network;
typedef PS5Network Network;
class PS5User;
typedef PS5User User;

#elif PLATFORM_XBOX_ONE

class ClipboardBase;
typedef ClipboardBase Clipboard;
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
class XboxOnePlatform;
typedef XboxOnePlatform Platform;
class Win32Thread;
typedef Win32Thread Thread;
class GDKWindow;
typedef GDKWindow Window;
class Win32Network;
typedef Win32Network Network;
class GDKUser;
typedef GDKUser User;

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
class GDKWindow;
typedef GDKWindow Window;
class Win32Network;
typedef Win32Network Network;
class GDKUser;
typedef GDKUser User;

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
class UnixNetwork;
typedef UnixNetwork Network;
class UserBase;
typedef UserBase User;

#elif PLATFORM_SWITCH

class ClipboardBase;
typedef ClipboardBase Clipboard;
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
class SwitchPlatform;
typedef SwitchPlatform Platform;
class SwitchThread;
typedef SwitchThread Thread;
class SwitchWindow;
typedef SwitchWindow Window;
class SwitchNetwork;
typedef SwitchNetwork Network;
class SwitchUser;
typedef SwitchUser User;

#elif PLATFORM_MAC

class MacClipboard;
typedef MacClipboard Clipboard;
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
class MacPlatform;
typedef MacPlatform Platform;
class AppleThread;
typedef AppleThread Thread;
class MacWindow;
typedef MacWindow Window;
class UnixNetwork;
typedef UnixNetwork Network;
class UserBase;
typedef UserBase User;

#elif PLATFORM_IOS

class ClipboardBase;
typedef ClipboardBase Clipboard;
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
class iOSPlatform;
typedef iOSPlatform Platform;
class AppleThread;
typedef AppleThread Thread;
class iOSWindow;
typedef iOSWindow Window;
class UnixNetwork;
typedef UnixNetwork Network;
class UserBase;
typedef UserBase User;

#else

#error Missing Types implementation!

#endif
