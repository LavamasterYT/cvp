# ConsoleVideoPlayer
 A video player but in the console for Windows!

![Showcase of Bad Apple playing](img/showcase.gif?raw=true "Bad Apple")

## Building
Building this project is easy, just run the following commands in the `ConsoleVideoProject` folder inside the `src` folder (you need .NET 6.0 to build):
```
$ dotnet restore
$ dotnet run
```
> This project only works on Windows due to relying on the Windows API to manipulate the terminal (more on this later).


## Running
Binaries will be generated in `bin\<configuration>\net6.0`. To play a video, just run `cvp <video path>`, or if you want to specify a custom palette, use `cvp -p <palette path> <video path>`.

### Playback
The dimension the video will be rendered will be based off the current size of the terminal (so feel free to experiment with different sizes). The video is downscaled and its colors are converted to just 16 based off of a predetermined palette. If you want to use a custom palette, create a new one by building the `CVPPaletteMaker` project, a GUI based palette maker.

#### Play/Pause
Press the spacebar to pause or play the video

#### Seeking
Left and right arrows rewind 10 seconds and fast forwards 10 seconds.

#### Volume
Use the = and - keys to increase/decrease the volume. (Volume can be seen in the bottom left next to the current time)

#### Exit
Press the escape key to exit

## Development
I was bored and I hadn't coded in a while, and I came up with the idea of a console video player after I saw videos of Bad Apple running on bizarre things like on [Desmos](https://www.youtube.com/watch?v=MVrNn5TuMkY) (pretty cool). I decided to challenge myself and try to not only just run Bad Apple on a console, but play any video file on a console. And thus, this project was born.

### Screen Renderer
First step I needed to solve was how was I going to draw to a console? I looked at a few possible solutions to this:
- [ncurses](https://invisible-island.net/ncurses/announce.html) - Allows you to manipulate the console pretty extensively, and is cross-platform.
- [Virtual Terminal Sequences](https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences) - Basically what **ncurses** wraps around I believe.
- [Console Manipulation](https://learn.microsoft.com/en-us/dotnet/api/system.console?view=net-7.0) - Since I ended up going with C# to program this (more on this later), I also considered this option. Allows for manipulating the console as well, but its extremely limited compared to **ncurses**. I do believe that with the way I *would've* implemented it, it could also be cross-platform.
- [Windows API](https://learn.microsoft.com/en-us/windows/console/console-functions) - Although limited to Windows only, I could directly modify the console buffer. Still extremely limited compared to **ncurses**, but is faster I think?

At the end, I decided to go with the **Windows API** for the following reasons:
- Although I really wanted to go with **ncurses** for cross-platform compatibility and its customization it provides, I wasn't able to find a way to efficiently draw to the screen, mainly due to the fact that changing the foreground and background colors have to be in pairs that are already created and initialized ([see](https://cboard.cprogramming.com/c-programming/64225-ncurses-colours-without-allocating-colour-pairs.html)). Although since in the end I ended up using the **Windows API** which only allows for 16 colors, I could've created a pair for each background and foreground combination. However, at the time, I was ambitious and wanted more than 16 colors, and considered doing the combination method to be tedious. I might go back and recreate it with **ncurses**, but for now, I'll stick with what I got.
- I actually did create a prototype using just **Virtual Terminal Sequences**, however it was extremely slow whenever I would draw to the screen.
- I used the **Console Manipulation** method before to create games and *draw pixels* on the console, however, it is slow compared to just directly modifying the console buffer, as this requires the cursor to move to every position, and then changing the color, then writing the character, and repeat for each pixel that changed from the previous frame.
- Although the biggest downside to using the **Windows API** was losing that cross-platform compatibility I would've had, it was still faster than the other methods I had tested.

Since the **Windows API** was used, I was originally gonna code it in C/C++, however decided in the end to code it in C# due to just me in general knowing the language more than C/C++.

So now that I chose a way to modify the console, I got to work implementing the renderer. The renderer consists of 3 parts: Creating the buffer; Setting *pixels*; Rendering the buffer.

Creating the buffer was simple. I used the [`CreateFileA`](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea) function to create a handle to the active screen buffer.

Drawing pixels on the screen was the hardest part of the renderer since it required a bit of thinking. By default, from my experience at least, the Windows console is rendered using the *Consolas* font, which is a mono font. If you've noticed, each character is around 2 rectangles tall (Example: `|▄▀█|`). This was powerful, because that means instead of rendering an image that is vertically stretched due to the vertical size of each character being bigger than the horizontal size, I can just use one of those squares and make it an even size. Even more so, I could render the character `|▀|`, change the foreground color, and the background color, and I got color in there too. If you want to see how it was implemented, check out the [`SetPixel()`](src/ConsoleVideoPlayer/Renderer.cs) function.

Rendering was simple. I used the [`WriteConsoleOutputW`](https://learn.microsoft.com/en-us/windows/console/writeconsoleoutput) function to just render the buffer to the console.

In the end, I was able to output pretty high quality frames to the console, while being kinda fast?

1080p:
![Showcase of a 1080p frame](img/1080p.png?raw=true "1080p")
4k:
![Showcase of a 4k frame](img/4k.png?raw=true "4K")
(Yes I know I said high quality, but for a console its high quality.)

### Frame processing and rendering
After that was done, it was time to figure out how to make the video displayable to the console. Since we are using the Windows console, we are limited to just the preset 16 colors and the low resolution.

First, however, we gotta open the video file. I used a wrapper for **OpenCV**, **OpenCvSharp**, and opened the video file. **OpenCV** allows me to go through the video frame by frame, and also get the frame located at a specific time.

From there, I was able to go through each frame of the video based on either frame number or frame in millisecond. Since I knew that this video player wasn't gonna run even at 30fps, I decided to just draw as many frames as we are able to output. I created a Stopwatch class and started it when we start playing the video. From there, in a loop until the Stopwatch reaches the end of the video (aka when the elapsed time is equal or greater to the duration of the video), I get the current frame based on the elapsed time, process the frame, and then render the frame, and repeat. A simplified version of this loop is shown here:
```c#
Stopwatch sw = new();
sw.Start();
while (sw.ElapsedMilliseconds < Duration.TotalMilliseconds)
{
    var frame = GetFrameAt(sw.ElapsedMilliseconds);
    ProcessFrame(frame);
    RenderFrame(frame);
}
```

Processing the frame was surprisingly easy. We first downscale the frame to fit the output resolution. In C# using GDI+ functions, it was as easy as just doing:
```c#
Bitmap downscaled = new Bitmap(sourceBitmap, newSize);
```
Converting the frame to just the 16 colors was also pretty simple. I was able to use the [Euclidean](https://en.wikipedia.org/wiki/Color_difference#Euclidean) algorithm to find the closest color based from the current color palette.

From there, I did some more things, then rendered the final output to the console, and boom!

### Audio
**NAudio** was a immense help in getting audio to work. It was just as simple as just extracting the audio part of the video, and then just playing that back while the video is being played.

### Input and UI
Input is handled on another thread. Each new frame that is drawn, it detects if a key is pressed and does the appropriate action. UI was also simple. I just scaled down the video to make fit the UI elements, and it was just simple writing characters to the screen and simple math for the progress bars.

### Conclusion
At the end, I was able to quickly make up this prototype based on everything above for playing back video in the console. I'm overall proud on how this turned out. I am aware that there are probably other existing projects either exactly like this or better than this, but I just made this for fun, and as kind of a learning experience. The code is pretty messy and I hope if I choose to, am able to clean it up the best I can.

## Credits
- [OpenCV](https://opencv.org/) and [OpenCvSharp](https://github.com/shimat/opencvsharp) for reading video frames
- [NAudio](https://github.com/naudio/NAudio) for audio playback
- [.NET](https://dotnet.microsoft.com/) for the .NET framework