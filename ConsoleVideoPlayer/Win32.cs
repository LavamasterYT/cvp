using Microsoft.Win32.SafeHandles;
using System.Runtime.InteropServices;

namespace ConsoleVideoPlayer;

public static class Win32
{
    [DllImport("kernel32.dll", SetLastError = true, EntryPoint = "WriteConsoleOutputW", CharSet = CharSet.Unicode)]
    public static extern bool WriteConsoleOutputW(SafeFileHandle hConsoleOutput, CHAR_INFO[] lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, ref PSMALL_RECT lpWriteRegion);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern SafeFileHandle CreateFile(string fileName, [MarshalAs(UnmanagedType.U4)] uint fileAccess, [MarshalAs(UnmanagedType.U4)] uint fileShare, IntPtr securityAttributes, [MarshalAs(UnmanagedType.U4)] FileMode creationDisposition, [MarshalAs(UnmanagedType.U4)] int flags, IntPtr template);

    [StructLayout(LayoutKind.Sequential)]
    public struct COORD
    {
        public short X;
        public short Y;

        public COORD(short X, short Y)
        {
            this.X = X;
            this.Y = Y;
        }
    }

    [StructLayout(LayoutKind.Explicit)]
    public struct CHAR_INFO
    {
        [FieldOffset(0)]
        public CharUnion Char;

        [FieldOffset(2)]
        public short Attributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PSMALL_RECT
    {
        public short Left;
        public short Top;
        public short Right;
        public short Bottom;
    }

    [StructLayout(LayoutKind.Explicit, CharSet = CharSet.Unicode)]
    public struct CharUnion
    {
        [FieldOffset(0)]
        public char UnicodeChar;

        [FieldOffset(0)]
        public byte AsciiChar;
    }
}
