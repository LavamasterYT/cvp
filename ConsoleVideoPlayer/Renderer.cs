using static ConsoleVideoPlayer.Win32;
using Microsoft.Win32.SafeHandles;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Drawing;

namespace ConsoleVideoPlayer;

public class Renderer : IDisposable
{
    public short Width { get; private set; }
    public short Height { get; private set; }

    private SafeFileHandle hWnd;

    private CHAR_INFO[] framebuffer;

    private readonly Dictionary<ConsoleColor, byte> ConAttr;

    private readonly COORD pos;
    private readonly COORD bufferSize;
    private PSMALL_RECT rect;

    public Renderer(short width, short height)
    {
        Width = width;
        Height = height;

        ConAttr = new Dictionary<ConsoleColor, byte>()
        {
            { ConsoleColor.Black, 0x0 },
            { ConsoleColor.Blue, 0x1 | 0x8 },
            { ConsoleColor.Cyan, 0x2 | 0x1 | 0x8 },
            { ConsoleColor.DarkBlue, 0x1 },
            { ConsoleColor.DarkCyan, 0x2 | 0x1 },
            { ConsoleColor.DarkGray, 0x1 | 0x2 | 0x4 },
            { ConsoleColor.DarkGreen, 0x2 },
            { ConsoleColor.DarkMagenta, 0x4 | 0x1 },
            { ConsoleColor.DarkRed, 0x4 },
            { ConsoleColor.DarkYellow, 0x4 | 0x2 },
            { ConsoleColor.Gray, 0x8 },
            { ConsoleColor.Green, 0x2 | 0x8 },
            { ConsoleColor.Magenta, 0x4 | 0x1 | 0x8 },
            { ConsoleColor.Red, 0x4 | 0x8 },
            { ConsoleColor.White, 0xF },
            { ConsoleColor.Yellow, 0x4 | 0x2 | 0x8 },
        };

        Console.CursorVisible = false;

        framebuffer = new CHAR_INFO[Width * (Height / 2)];
        for (int i = 0; i < framebuffer.Length; i++)
        {
            framebuffer[i] = new CHAR_INFO();
            framebuffer[i].Attributes = 0;
            framebuffer[i].Char = new CharUnion();
            framebuffer[i].Char.UnicodeChar = '▀';
        }

        pos = new COORD(0, 0);
        bufferSize = new COORD(Width, (short)(Height / 2));
        rect = new PSMALL_RECT() { Left = 0, Top = 0, Right = Width, Bottom = (short)(Height / 2) };

        hWnd = CreateFile("CONOUT$", 0x40000000, 2, IntPtr.Zero, FileMode.Open, 0, IntPtr.Zero);
    }

    public void SetPixel(int x, int y, ConsoleColor col)
    {
        byte color = ConAttr[col];

        int i = (x) + Width * (y / 2);

        if (y % 2 == 1)
        {
            framebuffer[i].Attributes = (byte)((framebuffer[i].Attributes & 0xFF0F) | color << 4);
        }
        else
        {
            framebuffer[i].Attributes = (byte)((framebuffer[i].Attributes & 0xFFF0) | color);
        }
    }

    public void SetChar(int x, int y, ConsoleColor foreground, ConsoleColor background, char character)
    {
        int i = (x) + Width * (y / 2);
        framebuffer[i] = new CHAR_INFO();
        framebuffer[i].Char = new CharUnion();
        framebuffer[i].Char.UnicodeChar = character;
        framebuffer[i].Attributes = (short)(ConAttr[foreground] | ConAttr[background] << 4);
    }

    public void WriteStr(int x, int y, ConsoleColor foreground, ConsoleColor background, string str)
    {
        for (int i = 0; i < str.Length; i++)
        {
            int index = (x + i) + Width * (y / 2);
            framebuffer[index] = new CHAR_INFO();
            framebuffer[index].Char = new CharUnion();
            framebuffer[index].Char.UnicodeChar = str[i];
            framebuffer[index].Attributes = (short)(ConAttr[foreground] | ConAttr[background] << 4);
        }
    }

    public void Clear(ConsoleColor col)
    {
        for (int i = 0; i < framebuffer.Length; i++)
        {
            framebuffer[i] = new CHAR_INFO();
            framebuffer[i].Attributes = (short)(ConAttr[col] | ConAttr[col] << 4);
            framebuffer[i].Char = new CharUnion();
            framebuffer[i].Char.UnicodeChar = '▀';
        }
    }

    public void Render()
    {
        rect = new PSMALL_RECT() { Left = 0, Top = 0, Right = Width, Bottom = (short)(Height / 2) };
        if (!WriteConsoleOutputW(hWnd, framebuffer, bufferSize, pos, ref rect))
        {
            Debug.WriteLine("Error occurred writing to console: 0x" + Marshal.GetLastWin32Error().ToString("X"));
        }
    }

    public void Dispose()
    {
        hWnd.Dispose();
    }

    ~Renderer()
    {
        Dispose();
    }
}