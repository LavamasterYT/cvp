using NAudio.Wave;
using OpenCvSharp;
using OpenCvSharp.Extensions;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Numerics;
using System.Security;
using System.Text;
using System.Threading.Tasks;

namespace ConsoleVideoPlayer
{
    public class VideoPlayer : IDisposable
    {
        /// <summary>
        /// The name of the file that is being played
        /// </summary>
        public string Filename { get; private set; }
        
        /// <summary>
        /// Horizontal output resolution
        /// </summary>
        public int VideoWidth { get; private set; }

        /// <summary>
        /// Vertical output resolution
        /// </summary>
        public int VideoHeight { get; private set; }

        /// <summary>
        /// Returns true if the video is finished playing
        /// </summary>
        public bool VideoFinished { get; private set; }

        /// <summary>
        /// Returns true if the video is paused (no frames are being rendered and audio is not being played)
        /// </summary>
        public bool Paused { get; private set; }

        private int videoXOffset; // The x offset of where to render the video, used to center the output

        private int consoleWidth; // The current width of the console, instead of using Console.WindowWidth, we
                                  // are instead assigning a variable to avoid as many unnecessary calls to that
                                  // property as we can

        private int consoleHeight; // The current height of the console, instead of using Console.WindowHeight, we
                                   // are instead assigning a variable to avoid as many unnecessary calls to that
                                   // property as we can

        private int uiPrgYPos; // Y position where the video progress is rendered
        private int uiStatusYPos; // Y position where pause and video time is rendered

        private int uiPrgTotal; // Total number of characters the progress bar takes up
        private int uiVolTotal; // Total number of characters the volume bar takes up

        private ConsoleKey keyPressed; // Key that is currently pressed

        private double frames; // Total video frame count
        private double fps; // Video frames per second

        private double swOffset; // Stopwatch offset, used for seeking
        private double msVideoPos; // Current video time position in milliseconds
        private double msDuration; // The duration of the video in milliseconds
        private string strDuration; // String representation of the video duration

        private Mat frame;
        private Compression compressor;
        private Renderer renderer;
        private VideoCapture src;
        private MemoryStream audioStream;
        private WaveStream wavStream;
        private WaveOutEvent audioPlayer;
        private Stopwatch stopwatch;
        private NativeInput input;

        public VideoPlayer(string file, string? palette = null)
        {
            // Initialize variables
            Filename = file;
            VideoFinished = false;

            consoleWidth = 0;
            consoleHeight = 0;
            VideoWidth = 0;
            VideoHeight = 0;
            videoXOffset = 0;
            uiVolTotal = 5;
            uiPrgYPos = 0;
            uiPrgTotal = 0;

            UpdateDimensions();

            swOffset = 0;
            msVideoPos = 0;

            keyPressed = ConsoleKey.NoName;

            // Initialize the compressor
            compressor = new Compression();
            if (palette is not null)
                compressor.SetPalette(palette);

            // Initialize the renderer to however big we can make it be
            renderer = new Renderer((short)Console.LargestWindowWidth,
                                    (short)((Console.LargestWindowHeight + 5) * 2));
            renderer.Clear(ConsoleColor.Black);
            renderer.Render();

            // Read video file and set properties
            src = new VideoCapture(Filename);
            frames = src.Get(VideoCaptureProperties.FrameCount);
            fps = src.Get(VideoCaptureProperties.Fps);

            var duration = TimeSpan.FromSeconds(frames / fps);
            strDuration = duration.ToString(@"m\:ss");
            msDuration = duration.TotalMilliseconds;
            frame = new Mat();

            // Extract audio from video file
            audioStream = new MemoryStream();
            using MediaFoundationReader reader = new MediaFoundationReader(Filename);

            if (reader.CanRead)
            {
                reader.Seek(0, SeekOrigin.Begin);
                WaveFileWriter.WriteWavFileToStream(audioStream, reader);
                audioStream.Seek(0, SeekOrigin.Begin);
            }

            wavStream = new WaveFileReader(audioStream);
            audioPlayer = new WaveOutEvent();
            audioPlayer.Init(wavStream);

            // Set up native input
            input = new NativeInput(new int[]
            {
                Win32.VK_SPACE,
                Win32.VK_ESCAPE,
                Win32.VK_LBUTTON,
                Win32.VK_LEFT,
                Win32.VK_RIGHT,
                Win32.VK_OEM_MINUS,
                Win32.VK_OEM_PLUS
            });

            stopwatch = new Stopwatch();
        }

        public void UpdateDimensions()
        {
            if (consoleWidth != Console.WindowWidth ||
                consoleHeight != Console.WindowHeight)
            {
                consoleWidth = Console.WindowWidth;
                consoleHeight = Console.WindowHeight;

                VideoWidth = (16 * (consoleHeight * 2)) / 9;
                VideoHeight = consoleHeight * 2 - 4;

                videoXOffset = (consoleWidth / 2) - (VideoWidth / 2);

                if (videoXOffset < 0)
                    videoXOffset = 0;

                uiPrgYPos = consoleHeight * 2 - 4;
                uiStatusYPos = consoleHeight * 2 - 2;
                uiPrgTotal = consoleWidth - 2;
            }
        }

        public void HandleInput()
        {
            input.HandleInput();

            if (((input.KeyStates[Win32.VK_SPACE].Item1 & Win32.KS_DOWN) == Win32.KS_DOWN &&
                (input.KeyStates[Win32.VK_SPACE].Item2 & Win32.KS_DOWN) != (input.KeyStates[Win32.VK_SPACE].Item1 & Win32.KS_DOWN)) ||
                ((input.KeyStates[Win32.VK_LBUTTON].Item1 & Win32.KS_DOWN) == Win32.KS_DOWN &&
                (input.KeyStates[Win32.VK_LBUTTON].Item2 & Win32.KS_DOWN) != (input.KeyStates[Win32.VK_LBUTTON].Item1 & Win32.KS_DOWN)))
            {
                if (!Paused)
                {
                    stopwatch.Stop();
                    audioPlayer.Pause();
                    Paused = true;
                }
                else
                {
                    stopwatch.Start();
                    audioPlayer.Play();
                    Paused = false;
                }
            }
            if ((input.KeyStates[Win32.VK_LEFT].Item1 & Win32.KS_DOWN) == Win32.KS_DOWN &&
                (input.KeyStates[Win32.VK_LEFT].Item2 & Win32.KS_DOWN) != (input.KeyStates[Win32.VK_LEFT].Item1 & Win32.KS_DOWN))
            {
                wavStream.Skip(-5);
                swOffset -= 5000;
            }
            if ((input.KeyStates[Win32.VK_RIGHT].Item1 & Win32.KS_DOWN) == Win32.KS_DOWN &&
                (input.KeyStates[Win32.VK_RIGHT].Item2 & Win32.KS_DOWN) != (input.KeyStates[Win32.VK_RIGHT].Item1 & Win32.KS_DOWN))
            {
                wavStream.Skip(5);
                swOffset += 5000;
            }
            if ((input.KeyStates[Win32.VK_OEM_PLUS].Item1 & Win32.KS_DOWN) == Win32.KS_DOWN &&
                (input.KeyStates[Win32.VK_OEM_PLUS].Item2 & Win32.KS_DOWN) != (input.KeyStates[Win32.VK_OEM_PLUS].Item1 & Win32.KS_DOWN))
            {
                if (audioPlayer.Volume + 0.2f > 1)
                    audioPlayer.Volume = 1;
                else
                    audioPlayer.Volume += 0.2f;
            }
            if ((input.KeyStates[Win32.VK_OEM_MINUS].Item1 & Win32.KS_DOWN) == Win32.KS_DOWN &&
                (input.KeyStates[Win32.VK_OEM_MINUS].Item2 & Win32.KS_DOWN) != (input.KeyStates[Win32.VK_OEM_MINUS].Item1 & Win32.KS_DOWN))
            {
                if (audioPlayer.Volume - 0.2f < 0)
                    audioPlayer.Volume = 0;
                else
                    audioPlayer.Volume -= 0.2f;
            }
            if ((input.KeyStates[Win32.VK_ESCAPE].Item1 & Win32.VK_ESCAPE) == Win32.VK_ESCAPE &&
                (input.KeyStates[Win32.VK_ESCAPE].Item2 & Win32.VK_ESCAPE) != (input.KeyStates[Win32.VK_ESCAPE].Item1 & Win32.VK_ESCAPE))
            {
                VideoFinished = true;
            }
        }

        public void DrawFrame()
        {
            src.Set(VideoCaptureProperties.PosMsec, msVideoPos);

            if (src.Read(frame) == false)
            {
                VideoFinished = true;
                return;
            }

            Bitmap bmp = frame.ToBitmap();
            Bitmap scaled = compressor.ResizeImage(bmp, VideoWidth, VideoHeight);

            byte[] frameData = compressor.ConvertFrame(scaled);
            
            bmp.Dispose();
            scaled.Dispose();

            renderer.Clear(ConsoleColor.Black);

            for (int i = 0; i < frameData.Length; i++)
            {
                int x = i % (VideoWidth / 2);
                x *= 2;
                int y = i / (VideoWidth / 2);

                renderer.SetPixel(x + videoXOffset, y, (ConsoleColor)((frameData[i] & 0xF0) >> 4));
                renderer.SetPixel(x + videoXOffset + 1, y, (ConsoleColor)(frameData[i] & 0x0F));
            }
        }

        public void DrawUI()
        {
            string strVideoPos = TimeSpan.FromMilliseconds(msVideoPos).ToString(@"m\:ss");

            // Draw progress bar border
            renderer.SetChar(0, uiPrgYPos, ConsoleColor.White, ConsoleColor.Black, '[');
            renderer.SetChar(consoleWidth - 1, uiPrgYPos, ConsoleColor.White, ConsoleColor.Black, ']');

            // Draw progress bar progress
            int progress = (int)((double)(msVideoPos / msDuration) * uiPrgTotal);
            for (int x = 0; x < uiPrgTotal; x++)
            {
                renderer.SetChar(1 + x, uiPrgYPos, ConsoleColor.White, ConsoleColor.Black, 
                                 x > progress ? '-' : '=');
            }

            // Draw play/pause and duration
            renderer.WriteStr(0, uiStatusYPos, ConsoleColor.White, ConsoleColor.Black, !Paused ? "\u23F8 " : "⏵︎");
            renderer.WriteStr(3, uiStatusYPos, ConsoleColor.White, ConsoleColor.Black, strVideoPos);

            // Draw volume
            renderer.SetChar(4 + strVideoPos.Length, uiStatusYPos, ConsoleColor.White, ConsoleColor.Black, '<');

            int volume = (int)(uiVolTotal * audioPlayer.Volume);
            for (int x = 0; x < uiVolTotal; x++)
            {
                renderer.SetChar(5 + strVideoPos.Length + x, uiStatusYPos, ConsoleColor.White, ConsoleColor.Black,
                                 x > volume ? '-' : '=');
            }

            renderer.SetChar(5 + uiVolTotal + strVideoPos.Length, uiStatusYPos, ConsoleColor.White, ConsoleColor.Black, '>');

            // Draw duration
            renderer.WriteStr(consoleWidth - strDuration.Length, uiStatusYPos, ConsoleColor.White, ConsoleColor.Black, strDuration);
            Console.Title = $"{Filename} - {strVideoPos}/{strDuration}";
        }

        public void Play()
        {
            stopwatch.Start();
            audioPlayer.Play();

            while (!VideoFinished)
            {
                UpdateDimensions();
                HandleInput();

                msVideoPos = stopwatch.ElapsedMilliseconds + swOffset;
                if (msVideoPos < 0)
                {
                    msVideoPos = 0;
                    swOffset = 0;
                    stopwatch.Restart();
                }

                DrawFrame();
                DrawUI();
                
                renderer.Render();

                Thread.Sleep(10);
            }
        }

        /// <summary>
        /// Properly disposes of all resources
        /// </summary>
        public void Dispose()
        {
            renderer.Dispose();
            src.Dispose();
            audioStream.Dispose();
            wavStream.Dispose();
            audioPlayer.Dispose();
            frame.Dispose();

            if (stopwatch.IsRunning)
                stopwatch.Stop();
        }

        ~VideoPlayer()
        {
            Dispose();
        }
    }
}
