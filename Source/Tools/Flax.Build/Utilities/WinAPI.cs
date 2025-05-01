// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.InteropServices;

namespace Flax.Build
{
    static class WinAPI
    {
        public static class dbghelp
        {
            [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool SymInitialize(IntPtr hProcess, string UserSearchPath, [MarshalAs(UnmanagedType.Bool)] bool fInvadeProcess);

            [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool SymCleanup(IntPtr hProcess);

            [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern ulong SymLoadModuleEx(IntPtr hProcess, IntPtr hFile, string ImageName, string ModuleName, long BaseOfDll, int DllSize, IntPtr Data, int Flags);

            [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool SymEnumerateSymbols64(IntPtr hProcess, ulong BaseOfDll, SymEnumerateSymbolsProc64 EnumSymbolsCallback, IntPtr UserContext);

            public delegate bool SymEnumerateSymbolsProc64(string SymbolName, ulong SymbolAddress, uint SymbolSize, IntPtr UserContext);
        }

        public static class kernel32
        {
            [DllImport("kernel32.dll", SetLastError = true)]
            public static extern IntPtr GlobalLock(IntPtr hMem);

            [DllImport("kernel32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool GlobalUnlock(IntPtr hMem);
        }

        public static class user32
        {
            [DllImport("user32.dll")]
            public static extern bool EmptyClipboard();

            [DllImport("user32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool OpenClipboard(IntPtr hWndNewOwner);

            [DllImport("user32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool CloseClipboard();

            [DllImport("user32.dll", SetLastError = true)]
            public static extern IntPtr SetClipboardData(uint uFormat, IntPtr data);

            [DllImport("user32.dll")]
            public static extern int MessageBoxA(IntPtr hWnd, string lpText, string lpCaption, uint uType);
        }

        // https://github.com/CopyText/TextCopy/blob/main/src/TextCopy/WindowsClipboard.cs
        public static void SetClipboard(string text)
        {
            user32.OpenClipboard(IntPtr.Zero);
            user32.EmptyClipboard();
            IntPtr hGlobal = default;
            try
            {
                var bytes = (text.Length + 1) * 2;
                hGlobal = Marshal.AllocHGlobal(bytes);
                var target = kernel32.GlobalLock(hGlobal);
                try
                {
                    Marshal.Copy(text.ToCharArray(), 0, target, text.Length);
                }
                finally
                {
                    kernel32.GlobalUnlock(target);
                }
                user32.SetClipboardData(13, hGlobal);
                hGlobal = default;
            }
            finally
            {
                if (hGlobal != default)
                {
                    Marshal.FreeHGlobal(hGlobal);
                }
                user32.CloseClipboard();
            }
        }

        // https://gist.github.com/nathan130200/53dec19cdcd895281fb568ec63e1275b
        public static class MessageBox
        {
            public enum Buttons
            {
                AbortRetryIgnore = 0x00000002,
                CancelTryIgnore = 0x00000006,
                Help = 0x00004000,
                Ok = 0x00000000,
                OkCancel = 0x00000001,
                RetryCancel = 0x00000005,
                YesNo = 0x00000004,
                YesNoCancel = 0x00000003,
            }

            public enum Result
            {
                Abort = 3,
                Cancel = 2,
                Continue = 11,
                Ignore = 5,
                No = 7,
                Ok = 1,
                Retry = 10,
                Yes = 6,
            }

            public enum DefaultButton : uint
            {
                Button1 = 0x00000000,
                Button2 = 0x00000100,
                Button3 = 0x00000200,
                Button4 = 0x00000300,
            }

            public enum Modal : uint
            {
                Application = 0x00000000,
                System = 0x00001000,
                Task = 0x00002000
            }

            public enum Icon : uint
            {
                Warning = 0x00000030,
                Information = 0x00000040,
                Question = 0x00000020,
                Error = 0x00000010
            }

            public static Result Show(string text)
            {
                return (Result)user32.MessageBoxA(IntPtr.Zero, text, "\0", (uint)Buttons.Ok);
            }

            public static Result Show(string text, string caption)
            {
                return (Result)user32.MessageBoxA(IntPtr.Zero, text, caption, (uint)Buttons.Ok);
            }

            public static Result Show(string text, string caption, Buttons buttons)
            {
                return (Result)user32.MessageBoxA(IntPtr.Zero, text, caption, (uint)buttons);
            }

            public static Result Show(string text, string caption, Buttons buttons, Icon icon)
            {
                return (Result)user32.MessageBoxA(IntPtr.Zero, text, caption, ((uint)buttons) | ((uint)icon));
            }

            public static Result Show(string text, string caption, Buttons buttons, Icon icon, DefaultButton button)
            {
                return (Result)user32.MessageBoxA(IntPtr.Zero, text, caption, ((uint)buttons) | ((uint)icon) | ((uint)button));
            }

            public static Result Show(string text, string caption, Buttons buttons, Icon icon, DefaultButton button, Modal modal)
            {
                return (Result)user32.MessageBoxA(IntPtr.Zero, text, caption, ((uint)buttons) | ((uint)icon) | ((uint)button) | ((uint)modal));
            }
        }
    }
}
