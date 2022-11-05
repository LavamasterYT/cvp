using OpenCvSharp;
using ConsoleVideoPlayer;
using NAudio.Wave;
using System.Drawing;
using OpenCvSharp.Extensions;
using System.Diagnostics;

if (args.Length != 1 && args.Length != 3)
{
    Console.WriteLine("Usage: cvp <input_file>");
    return;
}

string filename = "";
string palette = "";

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

int videoWidth = (16 * (Console.WindowHeight * 2)) / 9;
int videoHeight = Console.WindowHeight * 2 - 4;

int xoffset = (Console.WindowWidth / 2) - (videoWidth / 2);

if (xoffset < 0)
    xoffset = 0;

bool done = false;

ConsoleKey key = ConsoleKey.NoName;

Compression compressor = new Compression();

if (!string.IsNullOrEmpty(palette))
{
    compressor.SetPalette(palette);
}

using (Renderer renderer = new Renderer((short)Console.WindowWidth, (short)(Console.WindowHeight * 2)))
{
    renderer.Clear(ConsoleColor.Black);
    renderer.Render();

    using (VideoCapture src = new VideoCapture(filename))
    {
        using (MemoryStream audio = new MemoryStream())
        {
            using (MediaFoundationReader mediaReader = new MediaFoundationReader(filename))
            {
                if (mediaReader.CanRead)
                {
                    mediaReader.Seek(0, SeekOrigin.Begin);
                    WaveFileWriter.WriteWavFileToStream(audio, mediaReader);
                    audio.Seek(0, SeekOrigin.Begin);
                }
            }

            using (WaveStream stream = new WaveFileReader(audio))
            using (WaveOutEvent player = new WaveOutEvent())
            {
                player.Init(stream);

                Thread keyThread = new Thread(new ThreadStart(KeyLoop));
                keyThread.Start();

                Stopwatch sw = new Stopwatch();

                var frames = src.Get(VideoCaptureProperties.FrameCount);
                var fps = src.Get(VideoCaptureProperties.Fps);

                TimeSpan duration = TimeSpan.FromSeconds(frames / fps);
                string durationStr = duration.ToString(@"m\:ss");

                long swOffset = 0;
                bool paused = false;

                sw.Start();
                player.Play();

                while (!done)
                {
                    if (key == ConsoleKey.Spacebar)
                    {
                        if (!paused)
                        {
                            sw.Stop();
                            player.Pause();
                            paused = true;
                        }
                        else
                        {
                            sw.Start();
                            player.Play();
                            paused = false;
                        }
                    }
                    else if (key == ConsoleKey.LeftArrow)
                    {
                        stream.Skip(-10);
                        swOffset -= 10000;
                    }
                    else if (key == ConsoleKey.RightArrow)
                    {
                        stream.Skip(10);
                        swOffset += 10000;
                    }
                    else if (key == ConsoleKey.Escape)
                    {
                        done = true;
                    }
                    else if (key == ConsoleKey.OemMinus)
                    {
                        if (player.Volume - 0.2f < 0)
                            player.Volume = 0;
                        else
                            player.Volume -= 0.2f;
                    }
                    else if (key == ConsoleKey.OemPlus)
                    {
                        if (player.Volume + 0.2f > 1)
                            player.Volume = 1;
                        else
                            player.Volume += 0.2f;
                    }

                    key = ConsoleKey.NoName;

                    Mat img = new Mat();

                    long offset = sw.ElapsedMilliseconds + swOffset;
                    if (offset < 0)
                    {
                        offset = 0;
                        swOffset = 0;
                        sw.Restart();
                    }

                    src.Set(VideoCaptureProperties.PosMsec, offset);

                    if (src.Read(img) == false)
                    {
                        done = true;
                        break;
                    }

                    Bitmap bmp = img.ToBitmap();
                    img.Dispose();

                    Bitmap res = compressor.ResizeImage(bmp, videoWidth, videoHeight);

                    byte[] frame = compressor.ConvertFrame(res);
                    bmp.Dispose();
                    res.Dispose();

                    renderer.Clear(ConsoleColor.Black);

                    for (int i = 0; i < frame.Length; i++)
                    {
                        int x = i % (videoWidth / 2);
                        x *= 2;
                        int y = i / (videoWidth / 2);

                        renderer.SetPixel(x + xoffset, y, (ConsoleColor)((frame[i] & 0xF0) >> 4));
                        renderer.SetPixel(x + xoffset + 1, y, (ConsoleColor)(frame[i] & 0x0F));
                    }

                    renderer.SetChar(0, videoHeight + 1, ConsoleColor.White, ConsoleColor.Black, '[');
                    renderer.SetChar(renderer.Width - 1, videoHeight + 1, ConsoleColor.White, ConsoleColor.Black, ']');

                    int total = renderer.Width - 2;
                    int prg = (int)((double)(offset / duration.TotalMilliseconds) * total);

                    string tsOffset = TimeSpan.FromMilliseconds(offset).ToString(@"m\:ss");

                    for (int x = 0; x < total; x++)
                    {
                        if (x > prg)
                        {
                            renderer.SetChar(1 + x, videoHeight + 1, ConsoleColor.White, ConsoleColor.Black, '-');
                        }
                        else
                        {
                            renderer.SetChar(1 + x, videoHeight + 1, ConsoleColor.White, ConsoleColor.Black, '=');
                        }
                    }

                    renderer.WriteStr(0, videoHeight + 3, ConsoleColor.White, ConsoleColor.Black, !paused ? "\u23F8 " : "⏵︎");

                    renderer.WriteStr(3, videoHeight + 3, ConsoleColor.White, ConsoleColor.Black, tsOffset);

                    int volTotal = 5;
                    int vol = (int)(volTotal * player.Volume);

                    renderer.SetChar(4 + tsOffset.Length, videoHeight + 3, ConsoleColor.White, ConsoleColor.Black, '<');
                    for (int x = 0; x < volTotal; x++)
                    {
                        if (x > vol)
                        {
                            renderer.SetChar(5 + tsOffset.Length + x, videoHeight + 3, ConsoleColor.White, ConsoleColor.Black, '-');
                        }
                        else
                        {
                            renderer.SetChar(5 + tsOffset.Length + x, videoHeight + 3, ConsoleColor.White, ConsoleColor.Black, '=');
                        }
                    }
                    renderer.SetChar(10 + tsOffset.Length, videoHeight + 3, ConsoleColor.White, ConsoleColor.Black, '>');

                    renderer.WriteStr(renderer.Width - durationStr.Length, videoHeight + 3, ConsoleColor.White, ConsoleColor.Black, durationStr);

                    Console.Title = $"{filename} - {tsOffset}/{durationStr}";

                    renderer.Render();
                }
            }
        }
    }
}

Thread.Sleep(100);

Console.Clear();
Console.ResetColor();
Console.CursorVisible = true;

Environment.Exit(0);

void KeyLoop()
{
    while (!done)
    {
        key = Console.ReadKey(true).Key;
    }
}