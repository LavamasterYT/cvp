# cvp
 cvp (**C**onsole **V**ideo **P**layer), is a lightweight cross platform video player in the console!

![Showcase of Big Buck Bunny playing](img/showcase.gif?raw=true)

## Download

Binaries should be provided "hopefully" for the latest release.
I can't guarantee that the binaries will run successfully, so I recommend that you build this yourself.

## Building
Instructions basically follow this order:
- Download development tools (gcc, make, cmake, Visual Studio, etc)
- Install necessary libraries (FFmpeg, libavcodec, libavformat, libavutil, libswscale, sdl2)
- Build the source files inside the src directory and link with libraries above

### Debian/Ubuntu
Update apt:
```
sudo apt-get update
```
Download the necessary libararies and tools:
```
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libmpv-dev build-essential pkg-config
```
Finally, build using make:
```
make
```

### Other Linux Distros
Other distros are going to be different, but here is the gist:
- Install gcc, make, and pkg-config
- Install the necessary libraries (libavcodec, libavformat, libavutil, libswscale, libmpv, mpv, ffmpeg)
- Build using Cmake

### macOS
Install the Xcode command line tools:
```
xcode-select --install
```
Install the needed dependencies:
```
brew install ffmpeg sdl2 cmake
```
Finally, build using make:
```
make
```
If for some reason it doesn't want to build, do the following steps:
- With pkg-config, run `pkg-config --libs libavcodec libavformat libavutil libswscale mpv`
- Build by running `gcc *.c <output of pkg-config command> -o cvp`

> If you don't have brew, install it [here](https://brew.sh/).

### Windows
Unfourtunately, I could not find a way to build this on Windows with Makefile, however, you could use Visual Studio Community and `vcpkg` to build this project:
- Download and install [Visual Studio Community](https://visualstudio.microsoft.com/downloads/)
- Clone [vcpkg](https://github.com/microsoft/vcpkg) to a safe directory.
- Navigate to the directory you cloned vcpkg to, and run `bootstrap-vcpkg.bat`
- Install the FFmpeg libraries by running `vcpkg install ffmpeg`
- Download the [mpv](https://github.com/mpv-player/mpv) development libraries for Windows
- Integrate vcpkg with Visual Studio by running `vcpkg integrate install`
- Create a new project in Visual Studio, and add the source files to the project.
- Link mpv libraries to the project.
- Build is as you would normally build a project in Visual Studio.

## Usage
To play a video file, run the following command:
```
$ cvp <video file>
```
To play a video file in full RGB mode, run the following command:
```
$ cvp -f <video file>
```
To play a video file with audio:
```
$ cvp -a <video file>
```
More help is available by running:
```
$ cvp -h
```

> Some codecs (i think thats the issue? i still need to look into it) have trouble playing back. If you do, you can run `cvp` with multithreading by adding the `-t`  flag.

### Playback
The dimension the video will be rendered will be based off the current size of the terminal (so feel free to experiment with different sizes). The video will by default render in 16 colors for compatibility with most terminal emulators. Video frames are scaled down to the size of the terminal, then its colors are converted to the closest color in the 16 color palette that is defined in [renderer.c](https://github.com/LavamasterYT/cvp/blob/main/src/renderer.c#L19).

If your terminal supports truecolor, you can render in 24 bit by using the `-f` or `--full-color` flag, however, it may be slower due to having to push more data to the terminal.

## Development
Where to begin...

This is a rewrite of the old version I made in C#, if you want to look at how that worked, [look at the old repository](https://github.com/LavamasterYT/cvp/tree/180db8f0c03c20cdbccaf0f8848f757fa73888d8).

The problem with that version though was that it was not cross-platform, it only worked on Windows. For the longest time, I wanted to make it cross-platform, and make it better and faster. I finally got around to doing it, however, it had to be made in C and using FFmpeg.

Although I had no reason to, the [decoder](https://github.com/LavamasterYT/cvp/blob/main/src/decoder.h) and [renderer](https://github.com/LavamasterYT/cvp/blob/main/src/renderer.h) are able to be used independently of each other and this project. I don't for what reasons you would want to, but it's there.

### Decoder
The decoder part of the project is all located and implemented in the [decoder.c](https://github.com/LavamasterYT/cvp/blob/main/src/decoder.c) and [decoder.h](https://github.com/LavamasterYT/cvp/blob/main/src/decoder.h) files.

Decoding is being done using the [FFmpeg](https://ffmpeg.org/) libraries. This has the added advantage that it is able to support a large number of codecs. While I could have used OpenCV like I did last time (and it used OpenCV in the beginning), I think having more control over the decoding process was better, and this also removes any overhead that OpenCV may introduce (even though it may be little to none). This also makes building the project a lot more simpler and smaller.

### Renderer
The renderer is located and implemented in the [renderer.c](https://github.com/LavamasterYT/cvp/blob/main/src/renderer.c) and [renderer.h](https://github.com/LavamasterYT/cvp/blob/main/src/renderer.h) files.

Unlike the previous version which utilized the Windows API to manually write to the console buffer, this version uses ANSI sequences to render to the terminal. Although it may be slower, this has the advantage of being cross-platform and having more potential color support.

For rendering in palette mode, the output buffer is converted to the closest color in the 16 color palette using the euclidean algorithm. Then using ANSI sequences, it is written to the terminal. In truecolor mode, there is no conversion and instead each RGB value is rendered in the terminal. Due to more data being passed, it is slower than palette mode. However, it is still faster than the previous version. The program switches to the alternate screen buffer to prevent from messing with the main screen buffer.

With now using ANSI sequences to render to the terminal, it is possible to render video that looks as good as this:
![Demon Slayer in truecolor](img/truecolor1.jpg?raw=true)
![Demon Slayer in truecolor](img/truecolor2.jpg?raw=true)

As opposed to this (palette mode, also how old version looked):
![Demon Slayer in palette mode](img/palette1.png?raw=true)
![Demon Slayer in palette mode](img/palette2.png?raw=true)

## Credits
- [FFmpeg](https://ffmpeg.org/) for decoding functions