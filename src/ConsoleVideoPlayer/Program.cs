using ConsoleVideoPlayer;
using OpenCvSharp;
using OpenCvSharp.Extensions;
using System.Drawing;

string filename = "";
string? palette = null;

if (args.Length != 1 && args.Length != 3)
{
    Console.WriteLine("Usage: cvp <input_file>");
    return;
}

if (args.Length == 1)
{
    filename = args[0];
}
else
{
    if (args[0] == "-p")
    {
        filename = args[2];
        palette = args[1];
    }
}

if (!File.Exists(filename))
{
    Console.WriteLine("cvp: file does not exist.");
    return;
}

using (VideoPlayer player = new VideoPlayer(filename, palette))
{
    player.Play();
}

Thread.Sleep(100);

Console.Clear();
Console.ResetColor();
Console.CursorVisible = true;